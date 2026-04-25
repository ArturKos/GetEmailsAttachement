#include "mime_parser.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

std::string write_temp_file(const std::string &content)
{
    char path_template[] = "/tmp/mime_test_XXXXXX";
    int fd = mkstemp(path_template);
    if (fd < 0) {
        ADD_FAILURE() << "mkstemp failed";
        return {};
    }
    FILE *file = fdopen(fd, "wb");
    fwrite(content.data(), 1, content.size(), file);
    fclose(file);
    return std::string(path_template);
}

std::string read_file(const std::string &path)
{
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

} // namespace

TEST(MimeFindBoundary, ExtractsQuotedBoundary)
{
    char boundary[64];
    EXPECT_TRUE(mime_find_boundary("Content-Type: multipart/mixed; boundary=\"XYZ\"\r\n",
                                   boundary, sizeof(boundary)));
    EXPECT_STREQ(boundary, "--XYZ");
}

TEST(MimeFindBoundary, ExtractsUnquotedBoundary)
{
    char boundary[64];
    EXPECT_TRUE(mime_find_boundary("boundary=abc123\r\n", boundary, sizeof(boundary)));
    EXPECT_STREQ(boundary, "--abc123");
}

TEST(MimeFindBoundary, ReturnsFalseWhenAbsent)
{
    char boundary[64];
    EXPECT_FALSE(mime_find_boundary("Content-Type: text/plain\r\n",
                                    boundary, sizeof(boundary)));
}

TEST(MimeFindFilename, ExtractsQuotedFilename)
{
    char filename[64];
    EXPECT_TRUE(mime_find_filename("Content-Disposition: attachment; filename=\"report.pdf\"\r\n",
                                   filename, sizeof(filename)));
    EXPECT_STREQ(filename, "report.pdf");
}

TEST(MimeFindFilename, SanitizesPathSeparators)
{
    char filename[64];
    EXPECT_TRUE(mime_find_filename("filename=\"../../etc/passwd\"\r\n",
                                   filename, sizeof(filename)));
    EXPECT_STREQ(filename, ".._.._etc_passwd");
}

TEST(MimeFindFilename, ReturnsFalseWhenAbsent)
{
    char filename[64];
    EXPECT_FALSE(mime_find_filename("Subject: hello\r\n", filename, sizeof(filename)));
}

TEST(MimeIsHeaderLine, DetectsContentLine)
{
    EXPECT_TRUE(mime_is_header_line("Content-Type: application/octet-stream\r\n"));
    EXPECT_TRUE(mime_is_header_line(""));
    EXPECT_TRUE(mime_is_header_line(" continuation"));
    EXPECT_FALSE(mime_is_header_line("SGVsbG8sIFdvcmxkIQ=="));
}

TEST(MimeExtractAttachments, ExtractsSinglePdfAttachment)
{
    /* base64("Hello, World!") = SGVsbG8sIFdvcmxkIQ== */
    std::string message =
        "From: sender@example.com\r\n"
        "Content-Type: multipart/mixed; boundary=\"BOUND\"\r\n"
        "\r\n"
        "--BOUND\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Body text.\r\n"
        "--BOUND\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Disposition: attachment; filename=\"hello.bin\"\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "SGVsbG8sIFdvcmxkIQ==\r\n"
        "--BOUND--\r\n";

    std::string message_path = write_temp_file(message);
    FILE *file = fopen(message_path.c_str(), "rb");
    ASSERT_NE(file, nullptr);

    /* mime_extract_attachments writes to the current working directory.
     * Run it from a fresh temp dir so the test is hermetic. */
    char dir_template[] = "/tmp/mime_cwd_XXXXXX";
    ASSERT_NE(mkdtemp(dir_template), nullptr);
    char original_cwd[1024];
    ASSERT_NE(getcwd(original_cwd, sizeof(original_cwd)), nullptr);
    ASSERT_EQ(chdir(dir_template), 0);

    int extracted = mime_extract_attachments(file);
    fclose(file);

    EXPECT_EQ(extracted, 1);
    EXPECT_EQ(read_file("hello.bin"), "Hello, World!");

    unlink("hello.bin");
    ASSERT_EQ(chdir(original_cwd), 0);
    rmdir(dir_template);
    unlink(message_path.c_str());
}
