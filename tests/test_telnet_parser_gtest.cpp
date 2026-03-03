// test_telnet_parser_gtest.cpp
// Unit tests for TelnetParser — telnet IAC state machine, MCCP decompression,
// subnegotiation, protocol negotiation, NAWS, and MTTS terminal-type sequence.
//
// Null socket: m_connectionManager->m_pSocket is nullptr in all tests.
// SendPacket() guards null and returns silently. We verify state changes
// (IAC tracking arrays, flags, counters), not outgoing bytes.

#include "../src/text/line.h"
#include "../src/text/style.h"
#include "../src/world/telnet_parser.h"
#include "../src/world/world_document.h"
#include "fixtures/world_fixtures.h"
#include <gtest/gtest.h>
#include <span>
#include <vector>
#include <zlib.h>

class TelnetParserTest : public WorldDocumentTest {
  protected:
    void SetUp() override
    {
        WorldDocumentTest::SetUp();
        doc->m_telnetParser->reset();
        doc->m_display.utf8 = false;

        // Must initialize m_currentLine before feeding text bytes
        doc->m_currentLine = std::make_unique<Line>(1, 80, COLOUR_ANSI, WHITE, BLACK, false);
        auto style = std::make_unique<Style>();
        style->iLength = 0;
        style->iFlags = COLOUR_ANSI;
        style->iForeColour = WHITE;
        style->iBackColour = BLACK;
        style->pAction = nullptr;
        doc->m_currentLine->styleList.push_back(std::move(style));
    }

    void feed(std::initializer_list<unsigned char> bytes)
    {
        for (unsigned char b : bytes)
            doc->ProcessIncomingByte(b);
    }

    TelnetParser& tp()
    {
        return *doc->m_telnetParser;
    }

    // Produce zlib-format compressed data from plaintext.
    // deflateInit() (not Init2) produces zlib format matching inflateInit() used by ZlibStream.
    static std::vector<unsigned char> zlibDeflate(std::span<const unsigned char> plaintext,
                                                  bool finish = false)
    {
        z_stream zs{};
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        deflateInit(&zs, Z_DEFAULT_COMPRESSION);

        std::vector<unsigned char> out(plaintext.size() + 256);
        zs.next_in = const_cast<unsigned char*>(plaintext.data());
        zs.avail_in = static_cast<uInt>(plaintext.size());
        zs.next_out = out.data();
        zs.avail_out = static_cast<uInt>(out.size());

        deflate(&zs, finish ? Z_FINISH : Z_SYNC_FLUSH);
        out.resize(out.size() - zs.avail_out);
        deflateEnd(&zs);
        return out;
    }

    // Initialize TelnetParser's MCCP state (simulates what Handle_TELOPT_COMPRESS2 does)
    void activateMCCP()
    {
        tp().m_zlibStream.init();
        tp().m_CompressInput.assign(TELNET_COMPRESS_BUFFER_LENGTH, 0);
        tp().m_CompressOutput.assign(TELNET_COMPRESS_BUFFER_LENGTH, 0);
        tp().m_bCompress = true;
    }
};

// ========== Category 1: MCCP Decompression (direct decompressData calls) ==========

TEST_F(TelnetParserTest, Decompress_HappyPath)
{
    activateMCCP();

    const std::string plain = "Hello, MUD world!\n";
    std::vector<unsigned char> plainVec(plain.begin(), plain.end());
    auto compressed = zlibDeflate(plainVec);

    auto result =
        tp().decompressData({reinterpret_cast<const char*>(compressed.data()), compressed.size()});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), plain.size());
    EXPECT_EQ(std::string(result->begin(), result->end()), plain);
    EXPECT_TRUE(tp().m_bCompress);
    EXPECT_EQ(tp().m_nTotalCompressed, static_cast<qint64>(compressed.size()));
    EXPECT_EQ(tp().m_nTotalUncompressed, static_cast<qint64>(plain.size()));
}

TEST_F(TelnetParserTest, Decompress_LargePayload_BufferGrows)
{
    activateMCCP();
    // Shrink output buffer to exercise the multi-iteration loop path.
    // With a tiny buffer, decompressData must loop many times to collect all output.
    tp().m_CompressOutput.assign(16, 0);

    std::vector<unsigned char> big(500, 'A');
    auto compressed = zlibDeflate(big);

    auto result =
        tp().decompressData({reinterpret_cast<const char*>(compressed.data()), compressed.size()});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 500u);
    EXPECT_EQ(std::vector<unsigned char>(500, 'A'), *result);
    EXPECT_TRUE(tp().m_bCompress);
}

TEST_F(TelnetParserTest, Decompress_StreamEnd_DisablesCompression)
{
    activateMCCP();

    const std::string plain = "Final block";
    std::vector<unsigned char> plainVec(plain.begin(), plain.end());
    auto compressed = zlibDeflate(plainVec, /*finish=*/true);

    auto result =
        tp().decompressData({reinterpret_cast<const char*>(compressed.data()), compressed.size()});

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::string(result->begin(), result->end()), plain);
    EXPECT_FALSE(tp().m_bCompress);
}

TEST_F(TelnetParserTest, Decompress_CorruptData_ReturnsNullopt)
{
    activateMCCP();

    const std::vector<char> garbage = {0x00, 0x01, 0x02, 0x03, 0x7F, 0x42};
    auto result = tp().decompressData(garbage);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(tp().m_bCompress);
}

TEST_F(TelnetParserTest, Decompress_InputBufferFull_ReturnsNullopt)
{
    activateMCCP();
    // Leave only 5 bytes of space in input buffer
    tp().m_zlibStream.stream.avail_in = TELNET_COMPRESS_BUFFER_LENGTH - 5;

    const std::vector<char> data(6, 0x00);
    auto result = tp().decompressData(data);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(tp().m_bCompress);
}

TEST_F(TelnetParserTest, Decompress_PartialChunks_Reassembly)
{
    activateMCCP();

    const std::string plain = "Hello World from chunk test";
    std::vector<unsigned char> plainVec(plain.begin(), plain.end());
    auto compressed = zlibDeflate(plainVec);

    // Split at midpoint
    size_t mid = compressed.size() / 2;
    auto r1 = tp().decompressData({reinterpret_cast<const char*>(compressed.data()), mid});
    auto r2 = tp().decompressData(
        {reinterpret_cast<const char*>(compressed.data() + mid), compressed.size() - mid});

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(tp().m_bCompress);

    // Concatenate and verify
    std::string combined(r1->begin(), r1->end());
    combined.append(r2->begin(), r2->end());
    EXPECT_EQ(combined, plain);
}

// ========== Category 2: IAC State Machine ==========

TEST_F(TelnetParserTest, IAC_WILL_ECHO_SetsNoEcho)
{
    feed({TELNET_IAC, TELNET_WILL, TELOPT_ECHO_OPT});

    EXPECT_TRUE(tp().m_bNoEcho);
    EXPECT_TRUE(tp().m_bClient_got_IAC_WILL[TELOPT_ECHO_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_WILL, 1);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_ECHO_OPT]);
}

TEST_F(TelnetParserTest, IAC_WONT_ECHO_ClearsNoEcho)
{
    tp().m_bNoEcho = true;
    feed({TELNET_IAC, TELNET_WONT, TELOPT_ECHO_OPT});

    EXPECT_FALSE(tp().m_bNoEcho);
    EXPECT_TRUE(tp().m_bClient_got_IAC_WONT[TELOPT_ECHO_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_WONT, 1);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, IAC_DO_SGA_SetsTrackingBit)
{
    feed({TELNET_IAC, TELNET_DO, TELOPT_SGA_OPT});

    EXPECT_TRUE(tp().m_bClient_got_IAC_DO[TELOPT_SGA_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_DO, 1);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_WILL[TELOPT_SGA_OPT]);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, IAC_DONT_TerminalType)
{
    feed({TELNET_IAC, TELNET_DONT, TELOPT_TERMINAL_TYPE_OPT});

    EXPECT_TRUE(tp().m_bClient_got_IAC_DONT[TELOPT_TERMINAL_TYPE_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_DONT, 1);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, IAC_IAC_ReEntersIACState)
{
    // In MUSHclient behavior: IAC IAC → Phase_IAC sets c=IAC(0xFF), phase=NONE.
    // Recursive ProcessIncomingByte(0xFF) sees c==IAC and re-enters HAVE_IAC.
    // This means IAC IAC does NOT produce a data byte — it re-starts the IAC sequence.
    feed({TELNET_IAC, TELNET_IAC});

    EXPECT_EQ(tp().m_phase, Phase::HAVE_IAC);
    EXPECT_EQ(doc->m_currentLine->len(), 0); // No data byte produced

    // A subsequent NOP completes the IAC sequence
    feed({TELNET_NOP});
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, IAC_GA_WithConvertFlag)
{
    doc->m_spam.convert_ga_to_newline = true;
    feed({TELNET_IAC, TELNET_GO_AHEAD});

    EXPECT_EQ(doc->m_lineList.size(), 1u);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, IAC_GA_WithoutConvertFlag)
{
    // Default: convert_ga_to_newline is false
    feed({TELNET_IAC, TELNET_GO_AHEAD});

    EXPECT_EQ(doc->m_lineList.size(), 0u);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

// ========== Category 3: IAC Loop Prevention ==========

TEST_F(TelnetParserTest, SendIACDO_Idempotent)
{
    feed({TELNET_IAC, TELNET_WILL, TELOPT_SGA_OPT});
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_SGA_OPT]);

    // Second identical WILL — Send_IAC_DO guard should prevent re-send
    feed({TELNET_IAC, TELNET_WILL, TELOPT_SGA_OPT});
    EXPECT_EQ(tp().m_nCount_IAC_WILL, 2);                    // Received twice
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_SGA_OPT]); // Still set
}

TEST_F(TelnetParserTest, SendIACDO_Then_DONT_ClearsForward)
{
    // First: WILL SGA → client sends DO SGA
    feed({TELNET_IAC, TELNET_WILL, TELOPT_SGA_OPT});
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_SGA_OPT]);

    // Allow re-negotiation by clearing the sent flag
    tp().m_bClient_sent_IAC_DO[TELOPT_SGA_OPT] = false;

    // WONT SGA → client sends DONT SGA
    feed({TELNET_IAC, TELNET_WONT, TELOPT_SGA_OPT});
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DONT[TELOPT_SGA_OPT]);
    EXPECT_FALSE(tp().m_bClient_sent_IAC_DO[TELOPT_SGA_OPT]);
}

// ========== Category 4: Subnegotiation ==========

TEST_F(TelnetParserTest, Subneg_8KB_Cap)
{
    // Start subneg
    feed({TELNET_IAC, TELNET_SB, TELOPT_CHARSET_OPT});

    // Feed 8193 bytes without IAC SE — exceeds 8192 cap
    for (int i = 0; i < 8193; ++i) {
        doc->ProcessIncomingByte('A');
    }

    EXPECT_EQ(tp().m_phase, Phase::NONE);
    EXPECT_TRUE(tp().m_IAC_subnegotiation_data.isEmpty());
    EXPECT_EQ(tp().m_nCount_IAC_SB, 0); // No valid SE received
}

TEST_F(TelnetParserTest, Subneg_IAC_IAC_Escape)
{
    feed({TELNET_IAC, TELNET_SB, TELOPT_CHARSET_OPT});
    feed({'D', 'A', 'T', 'A'});
    feed({TELNET_IAC, TELNET_IAC}); // Escaped IAC inside subneg

    EXPECT_EQ(tp().m_phase, Phase::HAVE_SUBNEGOTIATION);
    EXPECT_EQ(tp().m_IAC_subnegotiation_data.size(), 5); // 4 data + 1 escaped IAC
    EXPECT_EQ(static_cast<unsigned char>(tp().m_IAC_subnegotiation_data[4]), 0xFF);
    EXPECT_EQ(tp().m_nCount_IAC_SB, 0); // No SE yet
}

TEST_F(TelnetParserTest, Subneg_Normal_End)
{
    feed({TELNET_IAC, TELNET_SB, TELOPT_MUD_SPECIFIC_OPT, 'h', 'i', TELNET_IAC, TELNET_SE});

    EXPECT_EQ(tp().m_nCount_IAC_SB, 1);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

// ========== Category 5: MCCP Negotiation ==========

TEST_F(TelnetParserTest, MCCPv2_Activation)
{
    // Phase 1: Server announces WILL COMPRESS2
    feed({TELNET_IAC, TELNET_WILL, TELOPT_COMPRESS2_OPT});

    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_COMPRESS2_OPT]);
    EXPECT_TRUE(tp().m_bSupports_MCCP_2);
    EXPECT_TRUE(tp().m_zlibStream.initialized);
    EXPECT_FALSE(tp().m_CompressInput.empty());
    EXPECT_FALSE(tp().m_CompressOutput.empty());

    // Phase 2: Server activates compression via subnegotiation
    feed({TELNET_IAC, TELNET_SB, TELOPT_COMPRESS2_OPT, TELNET_IAC, TELNET_SE});

    EXPECT_TRUE(tp().m_bCompress);
    EXPECT_EQ(tp().m_iMCCP_type, 2);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, MCCPv1_Activation)
{
    // Server offers MCCP v1
    feed({TELNET_IAC, TELNET_WILL, TELOPT_COMPRESS_OPT});

    // MCCP v1 activation: IAC SB COMPRESS WILL SE
    feed({TELNET_IAC, TELNET_SB, TELOPT_COMPRESS_OPT, TELNET_WILL, TELNET_SE});

    EXPECT_TRUE(tp().m_bCompress);
    EXPECT_EQ(tp().m_iMCCP_type, 1);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}

TEST_F(TelnetParserTest, MCCP_Disabled_Rejects)
{
    doc->m_bDisableCompression = true;
    feed({TELNET_IAC, TELNET_WILL, TELOPT_COMPRESS2_OPT});

    EXPECT_FALSE(tp().m_bCompress);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DONT[TELOPT_COMPRESS2_OPT]);
    EXPECT_FALSE(tp().m_bClient_sent_IAC_DO[TELOPT_COMPRESS2_OPT]);
}

// ========== Category 6: Protocol Flags ==========

TEST_F(TelnetParserTest, ZMP_Activation)
{
    doc->m_bUseZMP = true;
    feed({TELNET_IAC, TELNET_WILL, TELOPT_ZMP_OPT});

    EXPECT_TRUE(tp().m_bZMP);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_ZMP_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_WILL, 1);
}

TEST_F(TelnetParserTest, ZMP_NotActivated_WhenOff)
{
    // Default: m_bUseZMP is false
    feed({TELNET_IAC, TELNET_WILL, TELOPT_ZMP_OPT});

    EXPECT_FALSE(tp().m_bZMP);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DONT[TELOPT_ZMP_OPT]);
}

TEST_F(TelnetParserTest, ATCP_Activation)
{
    doc->m_bUseATCP = true;
    feed({TELNET_IAC, TELNET_WILL, TELOPT_ATCP_OPT});

    EXPECT_TRUE(tp().m_bATCP);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_ATCP_OPT]);
}

TEST_F(TelnetParserTest, MSP_Activation)
{
    doc->m_sound.use_msp = true;
    feed({TELNET_IAC, TELNET_WILL, TELOPT_MSP_OPT});

    EXPECT_TRUE(tp().m_bMSP);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_DO[TELOPT_MSP_OPT]);
}

// ========== Category 7: sendWindowSizes ==========

TEST_F(TelnetParserTest, NAWS_NullSocket_NoCrash)
{
    tp().sendWindowSizes(80);
    SUCCEED();
}

TEST_F(TelnetParserTest, NAWS_NotConnected_NoCrash)
{
    tp().m_bNAWS_wanted = true;
    tp().sendWindowSizes(80);
    SUCCEED();
}

TEST_F(TelnetParserTest, NAWS_Width255_NoCrash)
{
    tp().m_bNAWS_wanted = true;
    tp().sendWindowSizes(255);
    SUCCEED();
}

// ========== Category 8: MTTS Terminal Type ==========

TEST_F(TelnetParserTest, MTTS_Sequence_Increments)
{
    auto sendTTYPE = [&]() {
        feed({TELNET_IAC, TELNET_SB, TELOPT_TERMINAL_TYPE_OPT, 0x01, TELNET_IAC, TELNET_SE});
    };

    EXPECT_EQ(tp().m_ttype_sequence, 0);
    sendTTYPE(); // Responds with terminal ID, increments to 1
    EXPECT_EQ(tp().m_ttype_sequence, 1);
    sendTTYPE(); // Responds with "ANSI", increments to 2
    EXPECT_EQ(tp().m_ttype_sequence, 2);
    sendTTYPE(); // Responds with "MTTS 265", stays at 2
    EXPECT_EQ(tp().m_ttype_sequence, 2);
    EXPECT_EQ(tp().m_nCount_IAC_SB, 3);
}

TEST_F(TelnetParserTest, MTTS_ResetBy_DO)
{
    tp().m_ttype_sequence = 2;
    feed({TELNET_IAC, TELNET_DO, TELOPT_TERMINAL_TYPE_OPT});

    EXPECT_EQ(tp().m_ttype_sequence, 0);
    EXPECT_TRUE(tp().m_bClient_sent_IAC_WILL[TELOPT_TERMINAL_TYPE_OPT]);
    EXPECT_EQ(tp().m_nCount_IAC_DO, 1);
}

TEST_F(TelnetParserTest, MTTS_ResetBy_DONT)
{
    tp().m_ttype_sequence = 1;
    feed({TELNET_IAC, TELNET_DONT, TELOPT_TERMINAL_TYPE_OPT});

    EXPECT_EQ(tp().m_ttype_sequence, 0);
}

// ========== Category 9: Phase Integrity ==========

TEST_F(TelnetParserTest, Phase_ReturnsToNONE_AfterIAC)
{
    const unsigned char commands[] = {TELNET_NOP,
                                      TELNET_DATA_MARK,
                                      TELNET_ERASE_CHARACTER,
                                      TELNET_ERASE_LINE,
                                      TELNET_ARE_YOU_THERE,
                                      TELNET_ABORT_OUTPUT,
                                      TELNET_INTERRUPT_PROCESS,
                                      TELNET_BREAK,
                                      TELNET_SE};

    for (auto cmd : commands) {
        tp().m_phase = Phase::NONE;
        feed({TELNET_IAC, cmd});
        EXPECT_EQ(tp().m_phase, Phase::NONE) << "Failed for command: " << static_cast<int>(cmd);
    }
}

TEST_F(TelnetParserTest, EOR_BehavesLikeGA)
{
    doc->m_spam.convert_ga_to_newline = true;
    feed({TELNET_IAC, TELNET_EOR});

    EXPECT_EQ(doc->m_lineList.size(), 1u);
    EXPECT_EQ(tp().m_phase, Phase::NONE);
}
