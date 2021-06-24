#include "SerialAnalyzer.h"
#include "SerialAnalyzerSettings.h"
#include <AnalyzerChannelData.h>


SerialAnalyzer::SerialAnalyzer()
	:	Analyzer2(),
	  mSettings( new SerialAnalyzerSettings() ),
	  mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

SerialAnalyzer::~SerialAnalyzer()
{
	KillThread();
}

void SerialAnalyzer::ComputeSampleOffsets()
{
	ClockGenerator clock_generator;
	clock_generator.Init( mSettings->mBitRate, mSampleRateHz );

	mSampleOffsets.clear();
	
	U32 num_bits = mSettings->mBitsPerTransfer;

	if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
		num_bits++;

	mSampleOffsets.push_back( clock_generator.AdvanceByHalfPeriod( 1.5 ) );  //point to the center of the 1st bit (past the start bit)
	num_bits--;  //we just added the first bit.

	for( U32 i=0; i<num_bits; i++ )
	{
		mSampleOffsets.push_back( clock_generator.AdvanceByHalfPeriod() );
	}

	if( mSettings->mParity != AnalyzerEnums::None )
		mParityBitOffset = clock_generator.AdvanceByHalfPeriod();

	//to check for framing errors, we also want to check
	//1/2 bit after the beginning of the stop bit
	mStartOfStopBitOffset = clock_generator.AdvanceByHalfPeriod( 1.0 );  //i.e. moving from the center of the last data bit (where we left off) to 1/2 period into the stop bit

	//and 1/2 bit before end of the stop bit period
	mEndOfStopBitOffset = clock_generator.AdvanceByHalfPeriod( mSettings->mStopBits - 1.0 );  //if stopbits == 1.0, this will be 0
}


void SerialAnalyzer::SetupResults()
{
	//Unlike the worker thread, this function is called from the GUI thread
	//we need to reset the Results object here because it is exposed for direct access by the GUI, and it can't be deleted from the WorkerThread

	mResults.reset( new SerialAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mTxChannel );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mRxChannel );
}

void SerialAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	ComputeSampleOffsets();
	U32 num_bits = mSettings->mBitsPerTransfer;

	if (mSettings->mSerialMode != SerialAnalyzerEnums::Normal)
		num_bits++;

	if (mSettings->mTxInverted) {
		mTxData.bitHigh = BIT_LOW;
		mTxData.bitLow = BIT_HIGH;
		mTxData.inverted = true;
	}

	if (mSettings->mRxInverted) {
		mRxData.bitHigh = BIT_LOW;
		mRxData.bitLow = BIT_HIGH;
		mRxData.inverted = true;
	}

	U64 mask = 0x1ULL;
	for( U32 i=0; i<num_bits; i++ ) {
		bit_mask |= mask;
		mask <<= 1;
	}

	mRxData.type = Rx;
	mRxData.settingsChannel = &mSettings->mRxChannel;
	mRxData.gapSizeInSamples = (mSettings->mRxPacketMinGapIn_us) * (GetSampleRate() * 0.000001);
	mRxData.channel = GetAnalyzerChannelData( mSettings->mRxChannel );
	mRxData.channel->TrackMinimumPulseWidth();
	mRxData.nextSampleToProcess = mRxData.channel->GetSampleOfNextEdge();

	mTxData.type = Tx;
	mTxData.settingsChannel = &mSettings->mTxChannel;
	mTxData.gapSizeInSamples = mSettings->mTxPacketMinGapIn_us * (GetSampleRate() * 0.000001);
	mTxData.channel = GetAnalyzerChannelData( mSettings->mTxChannel );
	mTxData.channel->TrackMinimumPulseWidth();
	mTxData.nextSampleToProcess = mTxData.channel->GetSampleOfNextEdge();
	
	if( mTxData.channel->GetBitState() == mTxData.bitLow )
		mTxData.channel->AdvanceToNextEdge();

	if( mRxData.channel->GetBitState() == mRxData.bitLow )
		mRxData.channel->AdvanceToNextEdge();

	for( ; ; )
	{
		if ((mRxData.nextSampleToProcess < mTxData.nextSampleToProcess) || mTxData.lastPacketEndMarked) {
			WorkerThreadForChannel(&mRxData);
			ReportProgress(mRxData.nextSampleToProcess);
		} else {
			WorkerThreadForChannel(&mTxData);
			ReportProgress(mTxData.nextSampleToProcess);
		}

		if (mTxData.lastPacketEndMarked && mRxData.lastPacketEndMarked)
			SetThreadMustExit();
		CheckIfThreadShouldExit();
	}
}

void SerialAnalyzer::WorkerThreadForChannel(ChannelData *chData)
{
	if (!chData->channel->DoMoreTransitionsExistInCurrentData()) {
		if (!chData->lastPacketEndMarked) {
			mResults->AddMarker( chData->lastByteEnd, AnalyzerResults::Stop, *chData->settingsChannel );
			chData->lastPacketEndMarked = true;
		}
		return;
	}

	//we're starting high.  (we'll assume that we're not in the middle of a byte.
	chData->channel->AdvanceToNextEdge();

	//we're now at the beginning of the start bit.  We can start collecting the data.
	U64 frame_starting_sample = chData->channel->GetSampleNumber();

	U64 data = 0;
	bool parity_error = false;
	bool framing_error = false;
	bool mp_is_address = false;

	DataBuilder data_builder;
	data_builder.Reset( &data, mSettings->mShiftOrder, mSettings->mBitsPerTransfer );
	U64 marker_location = frame_starting_sample;

	if (chData->lastPacketStart == 0) {
		// mark as a new packet the very first edge
		mResults->AddMarker( frame_starting_sample, AnalyzerResults::Start, *chData->settingsChannel );
	} else {
		if (frame_starting_sample > (chData->lastPacketStart + chData->gapSizeInSamples)) {
			mResults->AddMarker( chData->lastByteEnd, AnalyzerResults::Stop, *chData->settingsChannel );
			mResults->AddMarker( frame_starting_sample, AnalyzerResults::Start, *chData->settingsChannel );
		}
	}

	for( U32 i=0; i<mSettings->mBitsPerTransfer; i++ )
	{
		chData->channel->Advance( mSampleOffsets[i] );
		data_builder.AddBit( chData->channel->GetBitState() );

		marker_location += mSampleOffsets[i];
		mResults->AddMarker( marker_location, AnalyzerResults::Dot, *chData->settingsChannel );
	}

	if (chData->inverted)
		data = (~data) & bit_mask;

	if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
	{
		//extract the MSB
		U64 msb = data >> (mSettings->mBitsPerTransfer - 1);
		msb &= 0x1;
		if( mSettings->mSerialMode == SerialAnalyzerEnums::MpModeMsbOneMeansAddress )
		{
			if( msb == 0x0 )
				mp_is_address = false;
			else
				mp_is_address = true;
		}
		if( mSettings->mSerialMode == SerialAnalyzerEnums::MpModeMsbZeroMeansAddress )
		{
			if( msb == 0x0 )
				mp_is_address = true;
			else
				mp_is_address = false;
		}
		//now remove the msb.
		data &= ( bit_mask >> 1 );
	}

	parity_error = false;

	if( mSettings->mParity != AnalyzerEnums::None )
	{
		chData->channel->Advance( mParityBitOffset );
		bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );

		if( mSettings->mParity == AnalyzerEnums::Even )
		{
			if( is_even == true )
			{
				if( chData->channel->GetBitState() != chData->bitLow ) //we expect a low bit, to keep the parity even.
					parity_error = true;
			}else
			{
				if( chData->channel->GetBitState() != chData->bitHigh ) //we expect a high bit, to force parity even.
					parity_error = true;
			}
		} else  //if( mSettings->mParity == AnalyzerEnums::Odd )
		{
			if( is_even == false )
			{
				if( chData->channel->GetBitState() != chData->bitLow ) //we expect a low bit, to keep the parity odd.
					parity_error = true;
			}else
			{
				if( chData->channel->GetBitState() != chData->bitHigh ) //we expect a high bit, to force parity odd.
					parity_error = true;
			}
		}

		marker_location += mParityBitOffset;
		mResults->AddMarker( marker_location, AnalyzerResults::Square, *chData->settingsChannel );
	}

	//now we must dermine if there is a framing error.
	framing_error = false;

	chData->channel->Advance( mStartOfStopBitOffset );

	if( chData->channel->GetBitState() != chData->bitHigh ) {
		framing_error = true;
	} else {
		U32 num_edges = chData->channel->Advance( mEndOfStopBitOffset );
		if( num_edges != 0 )
			framing_error = true;
	}

	if( framing_error == true )
	{
		marker_location += mStartOfStopBitOffset;
		mResults->AddMarker( marker_location, AnalyzerResults::ErrorX, *chData->settingsChannel );

		if( mEndOfStopBitOffset != 0 ) {
			marker_location += mEndOfStopBitOffset;
			mResults->AddMarker( marker_location, AnalyzerResults::ErrorX, *chData->settingsChannel );
		}
	}

	//ok now record the value!
	//note that we're not using the mData2 or mType fields for anything, so we won't bother to set them.
	Frame frame;
	frame.mStartingSampleInclusive = frame_starting_sample;
	frame.mEndingSampleInclusive = chData->channel->GetSampleNumber();
	chData->lastByteEnd = frame.mEndingSampleInclusive;
	frame.mData1 = data;
	frame.mFlags = 0;
	if( parity_error == true )
		frame.mFlags |= PARITY_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;

	if( framing_error == true )
		frame.mFlags |= FRAMING_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;

	if( mp_is_address == true )
		frame.mFlags |= MP_MODE_ADDRESS_FLAG;

	if( chData->type == Tx )
		frame.mFlags |= IS_TX;

	if ((chData->lastPacketStart == 0)
			|| (chData->lastPacketStart && (frame_starting_sample > (chData->lastPacketStart + chData->gapSizeInSamples)))) {
		frame.mFlags |= PACKET_START;
		chData->lastPacketStart = frame_starting_sample;
	}
	mResults->AddFrame( frame );
	mResults->CommitResults();

	if( framing_error == true )  //if we're still low, let's fix that for the next round.
	{
		if( chData->channel->GetBitState() == chData->bitLow )
			chData->channel->AdvanceToNextEdge();
	}

	if (chData->channel->DoMoreTransitionsExistInCurrentData()) {
		chData->nextSampleToProcess = chData->channel->GetSampleOfNextEdge();
	}
}

bool SerialAnalyzer::NeedsRerun()
{
	if( mSettings->mUseAutobaud == false )
		return false;

	//ok, lets see if we should change the bit rate, base on mShortestActivePulse

	U64 shortest_pulse = mTxData.channel->GetMinimumPulseWidthSoFar();
	U64 shortest_pulseRx = mRxData.channel->GetMinimumPulseWidthSoFar();

	shortest_pulse = shortest_pulse < shortest_pulseRx ? shortest_pulse : shortest_pulseRx;

	if( shortest_pulse == 0 )
		AnalyzerHelpers::Assert( "Alg problem, shortest_pulse was 0" );

	U32 computed_bit_rate = U32( double( mSampleRateHz ) / double( shortest_pulse ) );

	if( computed_bit_rate > mSampleRateHz )
		AnalyzerHelpers::Assert( "Alg problem, computed_bit_rate is higer than sample rate" );  //just checking the obvious...

	if( computed_bit_rate > (mSampleRateHz / 4) )
		return false; //the baud rate is too fast.
	if( computed_bit_rate == 0 )
	{
		//bad result, this is not good data, don't bother to re-run.
		return false;
	}

	U32 specified_bit_rate = mSettings->mBitRate;

	double error = double( AnalyzerHelpers::Diff32( computed_bit_rate, specified_bit_rate ) ) / double( specified_bit_rate );

	if( error > 0.1 )
	{
		mSettings->mBitRate = computed_bit_rate;
		mSettings->UpdateInterfacesFromSettings();
		return true;
	}else
	{
		return false;
	}
}

U32 SerialAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SerialAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* SerialAnalyzer::GetAnalyzerName() const
{
	return "Dual async serial";
}

const char* GetAnalyzerName()
{
	return "Dual async serial";
}

Analyzer* CreateAnalyzer()
{
	return new SerialAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
