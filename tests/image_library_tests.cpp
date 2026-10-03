#include <FreeImage.h>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <cassert>
#include <cstring>
#include <string>

static void TestImage(FREE_IMAGE_FORMAT format)
{
    FreeImage_Initialise(FALSE);
    for (auto input : {FIF_JPEG, FIF_PNG, FIF_BMP, FIF_TARGA, FIF_WEBP})
        assert(TRUE == FreeImage_FIFSupportsReading(input));
    FIBITMAP* bitmap = FreeImage_Allocate(2, 2, 32);
    assert(nullptr != bitmap);
    RGBQUAD color = {};
    color.rgbRed = 24;
    color.rgbGreen = 96;
    color.rgbBlue = 192;
    color.rgbReserved = 80;
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 2; ++x)
            assert(TRUE == FreeImage_SetPixelColor(bitmap, x, y, &color));
    FIMEMORY* memory = FreeImage_OpenMemory();
    assert(nullptr != memory);
    assert(TRUE == FreeImage_SaveToMemory(format, bitmap, memory, format == FIF_WEBP ? WEBP_LOSSLESS : 0));
    assert(TRUE == FreeImage_SeekMemory(memory, 0, SEEK_SET));
    FIBITMAP* decoded = FreeImage_LoadFromMemory(format, memory);
    assert(nullptr != decoded);
    assert(2u == FreeImage_GetWidth(decoded));
    assert(2u == FreeImage_GetHeight(decoded));
    RGBQUAD actual = {};
    assert(TRUE == FreeImage_GetPixelColor(decoded, 0, 0, &actual));
    assert(color.rgbRed == actual.rgbRed);
    assert(color.rgbGreen == actual.rgbGreen);
    assert(color.rgbBlue == actual.rgbBlue);
    assert(color.rgbReserved == actual.rgbReserved);
    FreeImage_Unload(decoded);
    FreeImage_CloseMemory(memory);
    FreeImage_Unload(bitmap);
    FreeImage_DeInitialise();
}

static void TestHTML()
{
    const char html[] = "<!doctype html><html><head><meta charset='utf-8'></head><body>"
        "<div class='floatHelp'>\xe5\x96\xb7\xe6\xbc\x86</div>"
        "<img id='ActualMedia' src='https://example.invalid/spray.jpg'></body></html>";
    htmlDocPtr document = htmlReadMemory(html, static_cast<int>(strlen(html)), nullptr, "UTF-8", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    assert(nullptr != document);
    xmlXPathContextPtr context = xmlXPathNewContext(document);
    assert(nullptr != context);
    xmlXPathObjectPtr images = xmlXPathEvalExpression(BAD_CAST "//img[@id='ActualMedia']", context);
    assert(nullptr != images);
    assert(1 == images->nodesetval->nodeNr);
    xmlChar* url = xmlGetProp(images->nodesetval->nodeTab[0], BAD_CAST "src");
    assert(0 == xmlStrcmp(BAD_CAST "https://example.invalid/spray.jpg", url));
    xmlFree(url);
    xmlXPathFreeObject(images);
    xmlXPathObjectPtr descriptions = xmlXPathEvalExpression(BAD_CAST "//div[@class='floatHelp']", context);
    assert(nullptr != descriptions);
    assert(1 == descriptions->nodesetval->nodeNr);
    xmlChar* text = xmlNodeGetContent(descriptions->nodesetval->nodeTab[0]);
    assert(0 == xmlStrcmp(BAD_CAST "\xe5\x96\xb7\xe6\xbc\x86", text));
    xmlFree(text);
    xmlXPathFreeObject(descriptions);
    xmlXPathFreeContext(context);
    xmlFreeDoc(document);
    xmlCleanupParser();
}

int main(int argc, char** argv)
{
    assert(2 == argc);
    const std::string scenario = argv[1];
    if (scenario == "HTML")
        TestHTML();
    else
    {
        assert(scenario == "PNG" || scenario == "WEBP");
        TestImage(scenario == "PNG" ? FIF_PNG : FIF_WEBP);
    }
    return 0;
}
