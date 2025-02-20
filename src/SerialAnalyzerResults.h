#pragma once

#include <stdint.h>
#include <AnalyzerResults.h>

#define FRAMING_ERROR_FLAG ( 1 << 0 )
#define PARITY_ERROR_FLAG ( 1 << 1 )
#define MP_MODE_ADDRESS_FLAG ( 1 << 2 )
#define IS_TX ( 1<<3 )
#define PACKET_START ( 1 << 4 )

class SerialAnalyzer;
class SerialAnalyzerSettings;
enum class ExportType : uint8_t;

class SerialAnalyzerResults : public AnalyzerResults
{
public:
	SerialAnalyzerResults( SerialAnalyzer* analyzer, SerialAnalyzerSettings* settings );
	virtual ~SerialAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	void GenerateCSV_OrTxt(const char* file, DisplayBase display_base);
	void GeneratePacketizedTxt(const char* file, bool addTimeStamps = false);
	void GenerateSingleChannelPacketizedTxt(const char* file, ExportType type);

protected:  //vars
	SerialAnalyzerSettings* mSettings;
	SerialAnalyzer* mAnalyzer;
};

