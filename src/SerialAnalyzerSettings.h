#pragma once

#include <stdint.h>
#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

namespace SerialAnalyzerEnums
{
	enum Mode { Normal, MpModeMsbZeroMeansAddress, MpModeMsbOneMeansAddress };
}

enum class ExportType : uint8_t {
	CSV_OrTxt,
	PacketizedText,
	PacketizedTextWithTimeStamps,
	TxOnly,
	TxOnlyWithTimeStamps,
	RxOnly,
	RxOnlyWithTimeStamps
};

class SerialAnalyzerSettings : public AnalyzerSettings
{
public:

	SerialAnalyzerSettings();
	virtual ~SerialAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mTxChannel;
	Channel mRxChannel;
	U32 mTxPacketMinGapIn_us = 10, mRxPacketMinGapIn_us = 10;
	bool mRxInverted = false;
	bool mTxInverted = false;

	U32 mBitRate;
	U32 mBitsPerTransfer;
	AnalyzerEnums::ShiftOrder mShiftOrder;
	double mStopBits;
	AnalyzerEnums::Parity mParity;
	bool mUseAutobaud;
	SerialAnalyzerEnums::Mode mSerialMode;

protected:
	std::unique_ptr< AnalyzerSettingInterfaceChannel >	mTxChannelInterface;
	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mTxPacketMinGapInterface;
	std::unique_ptr< AnalyzerSettingInterfaceChannel >	mRxChannelInterface;
	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mRxPacketMinGapInterface;

	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList > mBitsPerTransferInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mShiftOrderInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mStopBitsInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mParityInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mRxInvertedInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mTxInvertedInterface;
	std::unique_ptr< AnalyzerSettingInterfaceBool >	mUseAutobaudInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList >	mSerialModeInterface;
};
