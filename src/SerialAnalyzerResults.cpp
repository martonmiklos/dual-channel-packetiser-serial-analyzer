#include "SerialAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SerialAnalyzer.h"
#include "SerialAnalyzerSettings.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <cmath>
#include <iomanip>

SerialAnalyzerResults::SerialAnalyzerResults( SerialAnalyzer* analyzer, SerialAnalyzerSettings* settings )
	:	AnalyzerResults(),
	  mSettings( settings ),
	  mAnalyzer( analyzer )
{
}

SerialAnalyzerResults::~SerialAnalyzerResults()
{
}

void SerialAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	if (
			(channel == mSettings->mTxChannel && ((frame.mFlags & IS_TX) == 0))
			|| (channel == mSettings->mRxChannel && ((frame.mFlags & IS_TX) == IS_TX))) {
		return;
	}

	bool framing_error = false;
	if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
		framing_error = true;

	bool parity_error = false;
	if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
		parity_error = true;

	U32 bits_per_transfer = mSettings->mBitsPerTransfer;
	if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
		bits_per_transfer--;

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );

	char result_str[128];
	memset(result_str, 0, sizeof(result_str));

	//MP mode address case:
	bool mp_mode_address_flag = false;
	if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
	{
		mp_mode_address_flag = true;

		AddResultString( "A" );
		AddResultString( "Addr" );

		if( framing_error == false )
		{
			snprintf( result_str, sizeof(result_str), "Addr: %s", number_str );
			AddResultString( result_str );

			snprintf( result_str, sizeof(result_str), "Address: %s", number_str );
			AddResultString( result_str );

		}else
		{
			snprintf( result_str, sizeof(result_str), "Addr: %s (framing error)", number_str );
			AddResultString( result_str );

			snprintf( result_str, sizeof(result_str), "Address: %s (framing error)", number_str );
			AddResultString( result_str );
		}
		return;
	}

	//normal case:
	if( ( parity_error == true ) || ( framing_error == true ) )
	{
		AddResultString( "!" );

		snprintf( result_str, sizeof(result_str), "%s (error)", number_str );
		AddResultString( result_str );

		if( parity_error == true && framing_error == false )
			snprintf( result_str, sizeof(result_str), "%s (parity error)", number_str );
		else
			if( parity_error == false && framing_error == true )
				snprintf( result_str, sizeof(result_str), "%s (framing error)", number_str );
			else
				snprintf( result_str, sizeof(result_str), "%s (framing error & parity error)", number_str );

		AddResultString( result_str );

	}else
	{
		AddResultString( number_str );
	}
}

void SerialAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	auto type = static_cast<ExportType>(export_type_user_id);
	switch (type) {
	case ExportType::TxOnly:
	case ExportType::TxOnlyWithTimeStamps:
	case ExportType::RxOnly:
	case ExportType::RxOnlyWithTimeStamps:
		GenerateSingleChannelPacketizedTxt(file, type);
		break;
	case ExportType::CSV_OrTxt:
		GenerateCSV_OrTxt(file, display_base);
		break;
	case ExportType::PacketizedText:
		GeneratePacketizedTxt(file);
		break;
	case ExportType::PacketizedTextWithTimeStamps:
		GeneratePacketizedTxt(file, true);
		break;
	}
}

void SerialAnalyzerResults::GenerateCSV_OrTxt(const char* file, DisplayBase display_base)
{
	//export_type_user_id is only important if we have more than one export type.
	std::stringstream ss;

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	U64 num_frames = GetNumFrames();

	void* f = AnalyzerHelpers::StartFile( file );

	if( mSettings->mSerialMode == SerialAnalyzerEnums::Normal )
	{
		//Normal case -- not MP mode.
		ss << "Time [s],Value,Parity Error,Framing Error" << std::endl;

		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );
			
			//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128);

			ss << time_str << "," << number_str;

			if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
				ss << ",Error,";
			else
				ss << ",,";

			if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
				ss << "Error";


			ss << std::endl;

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );

			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
		}
	}else
	{
		//MP mode.
		ss << "Time [s],Packet ID,Address,Data,Framing Error" << std::endl;
		U64 address = 0;

		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );

			if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
			{
				address = frame.mData1;
				continue;
			}

			U64 packet_id = GetPacketContainingFrameSequential( i );

			//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char address_str[128];
			AnalyzerHelpers::GetNumberString( address, display_base, mSettings->mBitsPerTransfer - 1, address_str, 128 );

			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer - 1, number_str, 128 );
			if( packet_id == INVALID_RESULT_INDEX )
				ss << time_str << "," << "" << "," << address_str << "," << number_str << ",";
			else
				ss << time_str << "," << packet_id << "," << address_str << "," << number_str << ",";

			if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
				ss << "Error";

			ss << std::endl;

			AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
			ss.str( std::string() );


			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}


		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

SerialAnalyzer::ChannelType channelTypeFromFrame(const Frame &frame)
{
	return (frame.mFlags & IS_TX) ? SerialAnalyzer::Tx : SerialAnalyzer::Rx;
}

void SerialAnalyzerResults::GenerateSingleChannelPacketizedTxt(const char* file, ExportType type)
{
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 num_frames = GetNumFrames();
	if (num_frames) {
		U64 lastSkippedFrameIndex = 0;
		U64 lastSkippedFrameStartTime = 0;
		U64 frameCounter = 1;
		SerialAnalyzer::ChannelType processedChannel = SerialAnalyzer::Tx;
		if (type == ExportType::RxOnly || type == ExportType::RxOnlyWithTimeStamps)
			processedChannel = SerialAnalyzer::Rx;
		for( U32 processedFrameIndex = 0; processedFrameIndex < num_frames; processedFrameIndex++ )
		{
			if( processedFrameIndex != 0 ) {
				//below, we "continue" the loop rather than run to the end.  So we need to save to the file here.
				AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
				ss.str( std::string() );

				if( UpdateExportProgressAndCheckForCancel( processedFrameIndex, num_frames ) == true )
				{
					AnalyzerHelpers::EndFile( f );
					return;
				}
			}

			auto frame = GetFrame( processedFrameIndex );
			if (processedChannel == channelTypeFromFrame(frame)) {
				if (frame.mFlags & PACKET_START) {
					if (processedFrameIndex)
						ss << "\n";

					if (type == ExportType::RxOnlyWithTimeStamps
							|| type == ExportType::TxOnlyWithTimeStamps) {
						char time_str[128];
						AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, mAnalyzer->GetTriggerSample(), mAnalyzer->GetSampleRate(), time_str, 128 );
						ss << time_str << " ";
					}

					if (frame.mFlags & IS_TX) {
						ss << "TX";
					} else {
						ss << "RX";
					}
				}

				if (frame.mFlags & PARITY_ERROR_FLAG)
					ss << "pe";
				else if (frame.mFlags & FRAMING_ERROR_FLAG)
					ss << "fe";
				else
					ss << " " << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << frame.mData1;
			}
		}
	}
	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void SerialAnalyzerResults::GeneratePacketizedTxt(const char *file, bool addTimeStamps)
{
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 num_frames = GetNumFrames();
	if (num_frames) {
		U64 lastSkippedFrameIndex = 0;
		U64 lastSkippedFrameStartTime = 0;
		U64 frameCounter = 1;
		SerialAnalyzer::ChannelType processedChannel = GetFrame(0).mFlags & IS_TX ? SerialAnalyzer::Tx : SerialAnalyzer::Rx;
		for( U32 processedFrameIndex = 0; processedFrameIndex < num_frames; processedFrameIndex++ )
		{
			if( processedFrameIndex != 0 ) {
				//below, we "continue" the loop rather than run to the end.  So we need to save to the file here.
				AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
				ss.str( std::string() );

				if( UpdateExportProgressAndCheckForCancel( processedFrameIndex, num_frames ) == true )
				{
					AnalyzerHelpers::EndFile( f );
					return;
				}
			}

			auto frame = GetFrame( processedFrameIndex );
			if (processedChannel == channelTypeFromFrame(frame)) {
				if (frame.mFlags & PACKET_START) {
					printf("New packet at %lf ch %s\n",
						   frame.mStartingSampleInclusive / 500000000.0,
						   processedChannel == SerialAnalyzer::Tx ? "TX" : "RX");
					fflush(stdout);
					// the current frame belongs to a new packet
					// see if we skipped any packets on the opposite channel
					// which is before the current packet in time than the current
					if (lastSkippedFrameIndex != 0 && lastSkippedFrameStartTime < frame.mStartingSampleInclusive) {
						// save the current sample as skipped
						auto lastSkippedToRestore = lastSkippedFrameIndex;
						lastSkippedFrameIndex = processedFrameIndex;
						lastSkippedFrameStartTime = frame.mStartingSampleInclusive;

						// restore to the last skipped index
						processedFrameIndex = lastSkippedToRestore - 1;
						processedChannel = channelTypeFromFrame(GetFrame(processedFrameIndex + 1));
						printf("Jump back to %lf ch %s\n",
							   GetFrame(processedFrameIndex + 1).mStartingSampleInclusive / 500000000.0,
							   processedChannel == SerialAnalyzer::Tx ? "TX" : "RX");
						fflush(stdout);
						continue;
					}

					if (processedFrameIndex)
						ss << "\n";

					if (addTimeStamps) {
						char time_str[128];
						AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, mAnalyzer->GetTriggerSample(), mAnalyzer->GetSampleRate(), time_str, 128 );
						ss << time_str << " ";
					}

					if (frame.mFlags & IS_TX) {
						ss << "TX";
					} else {
						ss << "RX";
					}
				}

				if (frame.mFlags & PARITY_ERROR_FLAG)
					ss << "pe";
				else if (frame.mFlags & FRAMING_ERROR_FLAG)
					ss << "fe";
				else
					ss << " " << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << frame.mData1;
			} else {
				// mark only the very first frame on the opposite channel
				if (lastSkippedFrameIndex == 0 && (frame.mFlags & PACKET_START)) {
					// mark the first packet skipped on the opposite channel
					lastSkippedFrameIndex = processedFrameIndex;
					lastSkippedFrameStartTime = frame.mStartingSampleInclusive;
				}
			}

			if (processedFrameIndex == (num_frames - 1) && lastSkippedFrameIndex != 0) {
				// at the end export the rest of packets on the opposite channel (if left any)
				auto lastSkippedToRestore = lastSkippedFrameIndex;
				lastSkippedFrameIndex = 0;
				lastSkippedFrameStartTime = 0;

				// restore to the last skipped index
				processedFrameIndex = lastSkippedToRestore - 1;
				processedChannel = channelTypeFromFrame(GetFrame(processedFrameIndex + 1));
				printf("Jump back to %lf ch %s\n",
					   GetFrame(processedFrameIndex + 1).mStartingSampleInclusive / 500000000.0,
					   processedChannel == SerialAnalyzer::Tx ? "TX" : "RX");
				fflush(stdout);
				continue;
			}
		}
	}
	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void SerialAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	ClearTabularText();
	Frame frame = GetFrame( frame_index );

	bool framing_error = false;
	if( ( frame.mFlags & FRAMING_ERROR_FLAG ) != 0 )
		framing_error = true;

	bool parity_error = false;
	if( ( frame.mFlags & PARITY_ERROR_FLAG ) != 0 )
		parity_error = true;

	U32 bits_per_transfer = mSettings->mBitsPerTransfer;
	if( mSettings->mSerialMode != SerialAnalyzerEnums::Normal )
		bits_per_transfer--;

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, bits_per_transfer, number_str, 128 );

	char result_str[128];

	//MP mode address case:
	bool mp_mode_address_flag = false;
	if( ( frame.mFlags & MP_MODE_ADDRESS_FLAG ) != 0 )
	{
		mp_mode_address_flag = true;

		if( framing_error == false )
		{
			snprintf( result_str, sizeof(result_str), "Address: %s", number_str );
			AddTabularText( result_str );

		}else
		{
			snprintf( result_str, sizeof(result_str), "Address: %s (framing error)", number_str );
			AddTabularText( result_str );
		}
		return;
	}

	//normal case:
	if( ( parity_error == true ) || ( framing_error == true ) )
	{
		if( parity_error == true && framing_error == false )
			snprintf( result_str, sizeof(result_str), "%s (parity error)", number_str );
		else
			if( parity_error == false && framing_error == true )
				snprintf( result_str, sizeof(result_str), "%s (framing error)", number_str );
			else
				snprintf( result_str, sizeof(result_str), "%s (framing error & parity error)", number_str );

		AddTabularText( result_str );

	}else
	{
		AddTabularText( number_str );
	}
}

void SerialAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void SerialAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
