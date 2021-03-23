#include "SerialAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable: 4800) //warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)

SerialAnalyzerSettings::SerialAnalyzerSettings()
:	mTxChannel( UNDEFINED_CHANNEL ),
	mBitRate( 9600 ),
	mBitsPerTransfer( 8 ),
	mStopBits( 1.0 ),
	mParity( AnalyzerEnums::None ),
	mShiftOrder( AnalyzerEnums::LsbFirst ),
	mTxInverted( false ),
	mRxInverted( false ),
	mUseAutobaud( false ),
	mSerialMode( SerialAnalyzerEnums::Normal )
{
	mTxChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mTxChannelInterface->SetTitleAndTooltip( "Tx Channel", "Standard Async Serial" );
	mTxChannelInterface->SetChannel( mTxChannel );

	mRxChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mRxChannelInterface->SetTitleAndTooltip( "Rx Channel", "Standard Async Serial" );
	mRxChannelInterface->SetChannel( mRxChannel );

	mTxPacketMinGapInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mTxPacketMinGapInterface->SetTitleAndTooltip( "Tx packet min gap (us)",  "Specify the gap after the decoded bytes will be assigned to the next packet." );
	mTxPacketMinGapInterface->SetMax( 100000000 );
	mTxPacketMinGapInterface->SetMin( 1 );
	mTxPacketMinGapInterface->SetInteger( mTxPacketMinGapIn_us );

	mRxPacketMinGapInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mRxPacketMinGapInterface->SetTitleAndTooltip( "Rx packet min gap (us)",  "Specify the gap after the decoded bytes will be assigned to the next packet." );
	mRxPacketMinGapInterface->SetMax( 100000000 );
	mRxPacketMinGapInterface->SetMin( 1 );
	mRxPacketMinGapInterface->SetInteger( mRxPacketMinGapIn_us );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/s)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 100000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );

	mUseAutobaudInterface.reset( new AnalyzerSettingInterfaceBool() );
	mUseAutobaudInterface->SetTitleAndTooltip( "", "With Autobaud turned on, the analyzer will run as usual, with the current bit rate.  At the same time, it will also keep track of the shortest pulse it detects. \nAfter analyzing all the data, if the bit rate implied by this shortest pulse is different by more than 10% from the specified bit rate, the bit rate will be changed and the analysis run again." );
	mUseAutobaudInterface->SetCheckBoxText( "Use Autobaud" );
	mUseAutobaudInterface->SetValue( mUseAutobaud );

	mBitsPerTransferInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mBitsPerTransferInterface->SetTitleAndTooltip( "Bits per Frame", "Select the number of bits per frame" ); 
	for( U32 i = 1; i <= 64; i++ )
	{
		std::stringstream ss; 

		if( i == 1 )
			ss << "1 Bit per Transfer";
		else
			if( i == 8 )
				ss << "8 Bits per Transfer (Standard)";
			else
				ss << i << " Bits per Transfer";

		mBitsPerTransferInterface->AddNumber( i, ss.str().c_str(), "" );
	}
	mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );


	mStopBitsInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mStopBitsInterface->SetTitleAndTooltip( "Stop Bits", "Specify the number of stop bits." );
	mStopBitsInterface->AddNumber( 1.0, "1 Stop Bit (Standard)", "" );
	mStopBitsInterface->AddNumber( 1.5, "1.5 Stop Bits", "" );
	mStopBitsInterface->AddNumber( 2.0, "2 Stop Bits", "" );
	mStopBitsInterface->SetNumber( mStopBits ); 


	mParityInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mParityInterface->SetTitleAndTooltip( "Parity Bit", "Specify None, Even, or Odd Parity." );
	mParityInterface->AddNumber( AnalyzerEnums::None, "No Parity Bit (Standard)", "" );
	mParityInterface->AddNumber( AnalyzerEnums::Even, "Even Parity Bit", "" );
	mParityInterface->AddNumber( AnalyzerEnums::Odd, "Odd Parity Bit", "" ); 
	mParityInterface->SetNumber( mParity );


	mShiftOrderInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mShiftOrderInterface->SetTitleAndTooltip( "Significant Bit", "Select if the most significant bit or least significant bit is transmitted first" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::LsbFirst, "Least Significant Bit Sent First (Standard)", "" );
	mShiftOrderInterface->AddNumber( AnalyzerEnums::MsbFirst, "Most Significant Bit Sent First", "" );
	mShiftOrderInterface->SetNumber( mShiftOrder );


	mRxInvertedInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mRxInvertedInterface->SetTitleAndTooltip( "Signal inversion", "Specify if the RX signal is inverted" );
	mRxInvertedInterface->AddNumber( false, "Non Inverted (Standard)", "" );
	mRxInvertedInterface->AddNumber( true, "Inverted", "" );
	mRxInvertedInterface->SetNumber( mRxInverted );

	mTxInvertedInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mTxInvertedInterface->SetTitleAndTooltip( "Signal inversion", "Specify if the TX signal is inverted" );
	mTxInvertedInterface->AddNumber( false, "Non Inverted (Standard)", "" );
	mTxInvertedInterface->AddNumber( true, "Inverted", "" );
	mTxInvertedInterface->SetNumber( mTxInverted );

	enum Mode { Normal, MpModeRightZeroMeansAddress, MpModeRightOneMeansAddress, MpModeLeftZeroMeansAddress, MpModeLeftOneMeansAddress };
	mSerialModeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mSerialModeInterface->SetTitleAndTooltip( "Mode", "" );
	mSerialModeInterface->AddNumber( SerialAnalyzerEnums::Normal, "Normal", "" );
	mSerialModeInterface->AddNumber( SerialAnalyzerEnums::MpModeMsbZeroMeansAddress, "MP - Address indicated by MSB=0", "Multi-processor, 9-bit serial" );
	mSerialModeInterface->AddNumber( SerialAnalyzerEnums::MpModeMsbOneMeansAddress, "MDB - Address indicated by MSB=1 (TX only)", "Multi-drop, 9-bit serial" );
	mSerialModeInterface->SetNumber( mSerialMode );

	AddInterface( mTxChannelInterface.get() );
	AddInterface( mTxPacketMinGapInterface.get() );
	AddInterface( mTxInvertedInterface.get() );
	AddInterface( mRxChannelInterface.get() );
	AddInterface( mRxPacketMinGapInterface.get() );
	AddInterface( mRxInvertedInterface.get() );
	AddInterface( mBitRateInterface.get() );
	AddInterface( mBitsPerTransferInterface.get() );
	AddInterface( mStopBitsInterface.get() );
	AddInterface( mParityInterface.get() );
	AddInterface( mShiftOrderInterface.get() );
	AddInterface( mSerialModeInterface.get() );

	//AddExportOption( 0, "Export as text/csv file", "text (*.txt);;csv (*.csv)" );
	AddExportOption( static_cast<U32>(ExportType::CSV_OrTxt), "Export bytes as text/csv file" );
	AddExportExtension( static_cast<U32>(ExportType::CSV_OrTxt), "text", "txt" );
	AddExportExtension( static_cast<U32>(ExportType::CSV_OrTxt), "csv", "csv" );

	AddExportOption( static_cast<U32>(ExportType::PacketizedText), "Export as packetized txt" );
	AddExportExtension( static_cast<U32>(ExportType::PacketizedText), "text", "txt" );
	AddExportOption( static_cast<U32>(ExportType::PacketizedTextWithTimeStamps), "Export as packetized txt with timestamps" );
	AddExportExtension( static_cast<U32>(ExportType::PacketizedTextWithTimeStamps), "text", "txt" );

	AddExportOption( static_cast<U32>(ExportType::TxOnly), "Export as TX channel to packetized txt" );
	AddExportExtension( static_cast<U32>(ExportType::TxOnly), "text", "txt" );
	AddExportOption( static_cast<U32>(ExportType::TxOnlyWithTimeStamps), "Export as TX channel to packetized txt with timestamps" );
	AddExportExtension( static_cast<U32>(ExportType::TxOnlyWithTimeStamps), "text", "txt" );

	AddExportOption( static_cast<U32>(ExportType::RxOnly), "Export as RX channel to packetized txt" );
	AddExportExtension( static_cast<U32>(ExportType::RxOnly), "text", "txt" );
	AddExportOption( static_cast<U32>(ExportType::RxOnlyWithTimeStamps), "Export as RX channel to packetized txt with timestamps" );
	AddExportExtension( static_cast<U32>(ExportType::RxOnlyWithTimeStamps), "text", "txt" );

	ClearChannels();
	AddChannel( mTxChannel, "Serial", false );
}

SerialAnalyzerSettings::~SerialAnalyzerSettings()
{
}

bool SerialAnalyzerSettings::SetSettingsFromInterfaces()
{
	if( AnalyzerEnums::Parity( U32( mParityInterface->GetNumber() ) ) != AnalyzerEnums::None )
		if( SerialAnalyzerEnums::Mode( U32( mSerialModeInterface->GetNumber() ) ) != SerialAnalyzerEnums::Normal )
		{
			SetErrorText( "Sorry, but we don't support using parity at the same time as MP mode." );
			return false;
		}

	mTxChannel = mTxChannelInterface->GetChannel();
	mRxChannel = mRxChannelInterface->GetChannel();
	mTxPacketMinGapIn_us = mTxPacketMinGapInterface->GetInteger();
	mRxPacketMinGapIn_us = mRxPacketMinGapInterface->GetInteger();
	mBitRate = mBitRateInterface->GetInteger();
	mBitsPerTransfer = U32( mBitsPerTransferInterface->GetNumber() );
	mStopBits = mStopBitsInterface->GetNumber();
	mParity = AnalyzerEnums::Parity( U32( mParityInterface->GetNumber() ) );
	mShiftOrder =  AnalyzerEnums::ShiftOrder( U32( mShiftOrderInterface->GetNumber() ) );
	mTxInverted = bool( U32( mTxInvertedInterface->GetNumber() ) );
	mRxInverted = bool( U32( mRxInvertedInterface->GetNumber() ) );
	mUseAutobaud = mUseAutobaudInterface->GetValue();
	mSerialMode = SerialAnalyzerEnums::Mode( U32( mSerialModeInterface->GetNumber() ) );

	ClearChannels();
	AddChannel( mTxChannel, "Tx", true );
	AddChannel( mRxChannel, "Rx", true );

	return true;
}

void SerialAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mTxChannelInterface->SetChannel( mTxChannel );
	mRxChannelInterface->SetChannel( mRxChannel );
	mTxPacketMinGapInterface->SetInteger( mTxPacketMinGapIn_us );
	mRxPacketMinGapInterface->SetInteger( mRxPacketMinGapIn_us );
	mBitRateInterface->SetInteger( mBitRate );
	mBitsPerTransferInterface->SetNumber( mBitsPerTransfer );
	mStopBitsInterface->SetNumber( mStopBits );
	mParityInterface->SetNumber( mParity );
	mShiftOrderInterface->SetNumber( mShiftOrder );
	mTxInvertedInterface->SetNumber( mTxInverted );
	mRxInvertedInterface->SetNumber( mRxInverted );
	mUseAutobaudInterface->SetValue( mUseAutobaud );
	mSerialModeInterface->SetNumber( mSerialMode );
}

void SerialAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if( strcmp( name_string, "DualAsyncSerialAnalyzer" ) != 0 )
		AnalyzerHelpers::Assert( "DualAsyncSerialAnalyzer: Provided with a settings string that doesn't belong to us;" );

	text_archive >> mTxChannel;
	text_archive >> mRxChannel;
	text_archive >> mTxPacketMinGapIn_us;
	if (mTxPacketMinGapIn_us == 0)
		mTxPacketMinGapIn_us = 10;
	text_archive >> mRxPacketMinGapIn_us;
	if (mRxPacketMinGapIn_us == 0)
		mRxPacketMinGapIn_us = 10;
	text_archive >> mBitRate;
	text_archive >> mBitsPerTransfer;
	text_archive >> mStopBits;
	text_archive >> *(U32*)&mParity;
	text_archive >> *(U32*)&mShiftOrder;
	text_archive >> mRxInverted;
	text_archive >> mTxInverted;

	//check to make sure loading it actual works befor assigning the result -- do this when adding settings to an anylzer which has been previously released.
	bool use_autobaud;
	if( text_archive >> use_autobaud )
		mUseAutobaud = use_autobaud;

	SerialAnalyzerEnums::Mode mode;
	if( text_archive >> *(U32*)&mode )
		mSerialMode = mode;

	ClearChannels();
	AddChannel( mTxChannel, "Serial", true );

	UpdateInterfacesFromSettings();
}

const char* SerialAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "DualAsyncSerialAnalyzer";
	text_archive << mTxChannel;
	text_archive << mRxChannel;
	text_archive << mTxPacketMinGapIn_us;
	text_archive << mRxPacketMinGapIn_us;
	text_archive << mBitRate;
	text_archive << mBitsPerTransfer;
	text_archive << mStopBits;
	text_archive << mParity;
	text_archive << mShiftOrder;
	text_archive << mRxInverted;
	text_archive << mTxInverted;

	text_archive << mUseAutobaud;

	text_archive << mSerialMode;

	return SetReturnString( text_archive.GetString() );
}
