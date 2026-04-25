#include "base64.h"

#include <gtest/gtest.h>
#include <cstring>
#include <string>

static std::string decode_to_string(const char *input)
{
    unsigned char buffer[256];
    size_t written = 0;
    int rc = base64_decode(input, buffer, sizeof(buffer), &written);
    EXPECT_EQ(rc, 0) << "decode of \"" << input << "\" failed";
    return std::string(reinterpret_cast<char *>(buffer), written);
}

TEST(Base64, RfcVectorEmpty)
{
    EXPECT_EQ(decode_to_string(""), "");
}

TEST(Base64, RfcVectorF)
{
    EXPECT_EQ(decode_to_string("Zg=="), "f");
}

TEST(Base64, RfcVectorFo)
{
    EXPECT_EQ(decode_to_string("Zm8="), "fo");
}

TEST(Base64, RfcVectorFoo)
{
    EXPECT_EQ(decode_to_string("Zm9v"), "foo");
}

TEST(Base64, RfcVectorFoobar)
{
    EXPECT_EQ(decode_to_string("Zm9vYmFy"), "foobar");
}

TEST(Base64, IgnoresWhitespace)
{
    EXPECT_EQ(decode_to_string("Zm9v\r\nYmFy"), "foobar");
    EXPECT_EQ(decode_to_string("  Zm9v Ym Fy  "), "foobar");
}

TEST(Base64, RejectsInvalidCharacter)
{
    unsigned char buffer[16];
    size_t written = 0;
    EXPECT_EQ(base64_decode("Zm9v$Ymfy", buffer, sizeof(buffer), &written), -1);
}

TEST(Base64, RejectsTruncatedQuartet)
{
    unsigned char buffer[16];
    size_t written = 0;
    EXPECT_EQ(base64_decode("Zm9", buffer, sizeof(buffer), &written), -1);
}

TEST(Base64, RejectsInsufficientOutputBuffer)
{
    unsigned char buffer[2];
    size_t written = 0;
    EXPECT_EQ(base64_decode("Zm9vYmFy", buffer, sizeof(buffer), &written), -1);
}

TEST(Base64, DecodesBinaryPayload)
{
    /* "Hello, World!" -> SGVsbG8sIFdvcmxkIQ== */
    EXPECT_EQ(decode_to_string("SGVsbG8sIFdvcmxkIQ=="), "Hello, World!");
}
