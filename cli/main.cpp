#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <Magick++.h>
#include "shared.hpp"
#include "cpercep.hpp"
#include <wx/cmdline.h>

#include <wx/app.h>

class BrandonToolsApp : public wxAppConsole
{
    public:
        virtual bool OnInit();
        virtual void OnInitCmdLine(wxCmdLineParser& parser);
        virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
        bool Validate();
        bool DoLoadImages();
        bool DoHandleResize();
        bool DoCheckAndLabelImages();
        bool DoExportImages();
        int OnRun();
};

// Better command line interface than what I have now yo.
static const wxCmdLineEntryDesc cmd_descriptions[] =
{
    // For the noobs
    {wxCMD_LINE_SWITCH, "h", "help", "Displays help on command line parameters",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP},

    // For the hackers
    {wxCMD_LINE_SWITCH, "log", "log", "Debug logging", wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},

    // Modes
    {wxCMD_LINE_SWITCH, "mode0", "mode0", "Export image for use in mode0 (Not implemented do not use)",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, "mode3", "mode3", "Export image for use in mode3",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, "mode4", "mode4", "Export image for use in mode4",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, "sprites", "sprites", "Treat image as a GBA sprite sheet (Not implemented do not use)",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},

    // General helpful options
    {wxCMD_LINE_OPTION, "resize", "resize", "(Usage -resize=240,160) Resizes ALL images given to w,h",
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_OPTION, "transparent", "transparent",
        "(Usage -transparent=r,g,b) Makes the color r,g,b transparent note that r,g,b corresponds "
        "to a pixel in your input image. The range of r,g,b is [0,255]. This does not magically make "
        "things transparent you must ignore pixels matching the transparent color when drawing it "
        "(see generated header for MACRO that contains the transparent color in GBA colorspace). "
        "This does not work with DMA so don't even try kiddos.", wxCMD_LINE_VAL_STRING,
        wxCMD_LINE_PARAM_OPTIONAL },
    {wxCMD_LINE_SWITCH, "animated", "animated",
        "In addition to exporting multiple images, this parameter will create a final array that "
        "contains pointers to all of the other arrays. In the header file it will extern it so that "
        "it is available to any file that includes it. This is useful for animated images (duh). "
        "Note that all of the images you export will be in this array regardless of if you use an "
        "animated gif or multiple images or multiple animated gifs for that matter.",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},

    // Mode 4 options
    {wxCMD_LINE_SWITCH, "usegimp", "usegimp",
        "Only for mode4 exports.  Will run gimp and use its palette generation algorithm to generate the "
        "palette.  Only if you have copied makeindexed.scm in gimp's plugin directories.",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, "split", "split",
        "Only for mode4 exports.  Exports each individual image with its own palette.  Useful for sets of screens. "
        "Or videos (yes this program will convert an avi file into frames for you).",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_OPTION, "start", "start",
        "Only for mode4 exports. (Usage -start=X). Starts the palette off at index X. Useful if you "
        "have another image already exported that has X entries. You will load both palettes into palette mem.",
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_OPTION, "palette", "palette",
        "Only for mode4 exports. (Usage -palette=X). Will restrict the palette to X entries rather than 256. "
        "Useful when combined with -start.",
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},

    // Advanced Mode 4 options Use at your own risk.
    {wxCMD_LINE_OPTION, "weights", "weights",
        "Only for mode4 exports.  ADVANCED option use at your own risk. (Usage -weights=w1,w2,w3,w4) "
        "Fine tune? median cut algorithm.  w1 = volume, w2 = population, w3 = volume*population, w4 = error "
        "Affects image output quality for mode4, Range of w1-w4 is [0,100] but must sum up to 100 "
        "These values affects which colors are chosen by the median cut algorithm used to reduce the number "
        "of colors in the image.", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_OPTION, "dither", "dither",
        "Only for mode4 exports.  ADVANCED option use at your own risk. (Usage -dither=0 or -dither=1) "
        "Apply dithering after the palette is chosen.  Dithering makes the image look better by reducing "
        "large areas of one color as a result of reducing the number of colors in the image by adding noise.",
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_OPTION, "dither_level", "dither_level",
        "Only for mode4 exports.  ADVANCED option use at your own risk. (Usage -dither_level=num) "
        "Only applicable if -dither=1.  The range of num is [0,100]. This option affects how strong the "
        "dithering effect is by default it is set to 80.",
        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},

    // Forbidden options DO NOT USE under any circumstances.
    {wxCMD_LINE_SWITCH, "hide", "hide", "DO NOT USE", wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, "fullpalette", "fullpalette", "DO NOT USE", wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},

    {wxCMD_LINE_PARAM,  NULL, NULL, "output array name + input file(s)", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE},
    {wxCMD_LINE_NONE}
};

IMPLEMENT_APP(BrandonToolsApp);

// Parsed command line options
wxArrayString files;
// Debugging
bool logging = false;
// Modes
bool mode0 = false;
bool mode3 = false;
bool mode4 = false;
bool sprites = false;
// Helpful options
wxString resize = "";
wxString transparent = "";
bool animated = false;
// Mode 4 options
bool usegimp = false;
bool split_palette = false;
long start = 0;
long palette_size = 256;
wxString weights = "";
long dither = 1;
long dither_level = 80;
bool hide = false;
bool full_palette = false;

// All of the read in command line flags will be in this structure.
ExportParams eparams;

/** OnInit
  *
  * Initializes the program
  */
bool BrandonToolsApp::OnInit()
{
    if (!wxAppConsole::OnInit())
    {
        printf("A problem occurred, please report this and give any images the command line that caused this\n");
        return false;
    }

    return true;
}

/** @brief OnCmdLineParsed
  *
  * @todo: document this function
  */
bool BrandonToolsApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    logging = parser.Found(_("log"));
    mode0 = parser.Found(_("mode0"));
    mode3 = parser.Found(_("mode3"));
    mode4 = parser.Found(_("mode4"));
    sprites = parser.Found(_("sprites"));

    parser.Found(_("resize"), &resize);
    parser.Found(_("transparent"), &transparent);
    animated = parser.Found(_("animated"));

    usegimp = parser.Found(_("usegimp"));
    split_palette = parser.Found(_("split"));
    parser.Found(_("start"), &start);
    parser.Found(_("palette"), &palette_size);

    parser.Found(_("weights"), &weights);
    parser.Found(_("dither"), &dither);
    parser.Found(_("dither_level"), &dither_level);

    hide = parser.Found(_("hide"));
    full_palette = parser.Found(_("fullpalette"));

    // get unnamed parameters
    for (unsigned int i = 0; i < parser.GetParamCount(); i++)
    {
        files.Add(parser.GetParam(i));
    }

    // Do error checking later
    return true;
}

/** @brief OnInitCmdLine
  *
  * @todo: document this function
  */
void BrandonToolsApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetLogo(wxString::Format(_("brandontools version %s"), AutoVersion::FULLVERSION_STRING));
    parser.SetDesc(cmd_descriptions);
    parser.SetSwitchChars (_("-"));
}

bool BrandonToolsApp::Validate()
{
    // default params
    eparams.width = -1;
    eparams.height = -1;
    eparams.transparent_color = -1;
    eparams.animated = animated;

    eparams.offset = start;
    eparams.weights[0] = 25;
    eparams.weights[1] = 25;
    eparams.weights[2] = 25;
    eparams.weights[3] = 25;
    eparams.dither = dither;
    eparams.dither_level = dither_level / 100.0f;
    eparams.palette = palette_size;
    eparams.fullpalette = full_palette;
    eparams.split = split_palette;

    // Mode check
    if (mode0 + mode3 + mode4 + sprites != 1)
    {
        printf("[FATAL] Only 1 of -modeXXX and -sprites can be set at a time or none set.\n");
        return false;
    }

    if (mode0) eparams.mode = 0;
    if (mode3) eparams.mode = 3;
    if (mode4) eparams.mode = 4;
    if (sprites) eparams.mode = 7;

    if (files.size() <= 1)
    {
        printf("[FATAL] You must specify an output array/filename and a list of image files you want to export.\n");
        return false;
    }

    eparams.name = files[0];
    for (unsigned int i = 1; i < files.size(); i++)
        // Validate file's names here.
        eparams.files.push_back(files[i].ToStdString());

    if (!resize.IsEmpty())
    {
        std::vector<std::string> tokens;
        split(resize.ToStdString(), ',', tokens);
        if (tokens.size() != 2)
        {
            printf("[FATAL] error parsing -resize only need 2 comma separated values, %d given\n.", tokens.size());
            return false;
        }
        eparams.width = atoi(tokens[0].c_str());
        eparams.height = atoi(tokens[1].c_str());
        if (eparams.files.size() > 1)
        {
            printf("[WARNING] multiple files given, reminder they will ALL be resized.\n");
        }
        if (eparams.width <= 0 || eparams.height <= 0)
        {
            printf("[FATAL] bad width or height %d,%d.\n", eparams.width, eparams.height);
            return false;
        }
    }

    if (!transparent.IsEmpty())
    {
        std::vector<std::string> tokens;
        split(transparent.ToStdString(), ',', tokens);
        if (tokens.size() != 3)
        {
            printf("[FATAL] error parsing -transparent\n.");
            return false;
        }
        int r = atoi(tokens[0].c_str());
        int g = atoi(tokens[1].c_str());
        int b = atoi(tokens[2].c_str());
        r = (int)round(r*31)/255;
        g = (int)round(g*31)/255;
        b = (int)round(b*31)/255;

        if (r > 31 || r < 0 || g < 0 || g > 31 || b < 0 || b > 31)
            printf("[WARNING] -transparent one of r,g,b outside range continuing...\n");

        eparams.transparent_color = b << 10 | g << 5 | r;
        header.SetTransparent(eparams.transparent_color);
    }

    if (mode4 && animated)
    {
        printf("[WARNING] animated is not implemented for mode4 exports if you really want this let me know.\n");
        eparams.animated = animated;
    }

    if (eparams.offset >= 256)
    {
        printf("[WARNING] -start palette offset set to >= 256.\n");
    }

    if (!weights.IsEmpty())
    {
        std::vector<std::string> tokens;
        split(weights.ToStdString(), ',', tokens);
        if (tokens.size() != 4)
        {
            printf("[FATAL] error parsing -weights\n.");
            return false;
        }
        int p = atoi(tokens[0].c_str());
        int v = atoi(tokens[1].c_str());
        int pv = atoi(tokens[2].c_str());
        int error = atoi(tokens[3].c_str());

        if (p < 0 || v < 0 || pv < 0 || error < 0 || (p+v+pv+error != 100))
            printf("[WARNING] -weights total does not sum up to 100 or invalid value given...\n");

        eparams.weights[0] = p;
        eparams.weights[1] = v;
        eparams.weights[2] = pv;
        eparams.weights[3] = error;
    }

    if (eparams.palette > 256)
    {
        printf("[WARNING] trying to make palette > 256.\n");
    }

    return true;
}

bool BrandonToolsApp::DoLoadImages()
{
    try
    {
        for (unsigned int i = 0; i < eparams.files.size(); i++)
        {
            if (!usegimp)
            {
                readImages(&eparams.images, eparams.files[i]);
            }
            else
            {
                char command[1024];
                std::string filename = eparams.files[i];
                Chop(filename);
                filename = "/tmp/" + filename;
                snprintf(command, 1024, "gimp -d -i -b \"(script-fu-make-indexed \\\"%s\\\" %d \\\"%s\\\")\" -b \"(gimp-quit 0)\" 2> /dev/null",
                         eparams.files[i].c_str(), eparams.palette, filename.c_str());
                int ret = system(command);
                if (ret != 0) printf("[WARNING] You specified to use gimp however gimp has failed for some reason\n");
                if (ret != 0)
                    readImages(&eparams.images, eparams.files[i]);
                else
                    readImages(&eparams.images, filename);
                if (ret == 0)
                    printf("[INFO] GIMP Successfully exported image!");
            }
        }
        for (unsigned int i = 0; i < eparams.files.size(); i++)
            Chop(eparams.files[i]);
    }
    catch( Magick::Exception &error_ )
    {
        printf("%s\n", error_.what());
        printf("Export failed check the image(s) you are trying to load into the program");
        return false;
    }

    return true;
}

bool BrandonToolsApp::DoHandleResize()
{
    if (eparams.width != -1 && eparams.height != -1)
    {
        for (unsigned int i = 0; i < eparams.images.size(); i++)
        {
            Magick::Geometry geom(eparams.width, eparams.height);
            geom.aspect(true);
            eparams.images[i].resize(geom);
        }
    }

    if (eparams.mode != 0 && eparams.mode != 7)
    {
        for (unsigned int i = 0; i < eparams.images.size(); i++)
        {
            if (eparams.images[i].columns() > 240) printf(WARNING_WIDTH, eparams.images[i].baseFilename().c_str(), (int)eparams.images[i].columns());
            if (eparams.images[i].rows() > 160) printf(WARNING_HEIGHT, eparams.images[i].baseFilename().c_str(), (int)eparams.images[i].rows());
        }
    }

    return true;
}

bool BrandonToolsApp::DoCheckAndLabelImages()
{
    for (unsigned int i = 0; i < eparams.images.size(); i++)
    {
        bool isAnim = false;

        if (i + 1 != eparams.images.size())
            isAnim = eparams.images[i].scene() < eparams.images[i + 1].scene() || eparams.images[i].scene() != 0;
        else if (eparams.images.size() != 1)
            isAnim = eparams.images[i - 1].scene() < eparams.images[i].scene();
        header.AddImage(eparams.images[i], isAnim);
        eparams.images[i].label(isAnim ? "T" : "F");
    }

    std::map<std::string, int> used_times;
    std::set<std::string> output_names;
    for (unsigned int i = 0; i < eparams.images.size(); i++)
    {
        std::string filename = eparams.images[i].baseFilename();
        Chop(filename);
        int last = filename.rfind('.');
        filename = Sanitize(filename.substr(0, last));
        used_times[filename] += 1;
        if (used_times[filename] > 1)
        {
            std::stringstream out;
            out << filename;
            out << used_times[filename];
            std::string saved = out.str();
            std::string attempt = saved;
            int fails = 1;
            while (output_names.find(attempt) != output_names.end())
            {
                std::stringstream out2;
                out2 << saved << "_" << fails;
                attempt = out2.str();
                fails++;
            }
            filename = attempt;
        }
        else if (output_names.find(filename) != output_names.end())
        {
            // Was already used due to if statement above, and another filename had a name that was generated from the rollover
            // This if statement gives it a nice name as filename_1, filename_2.
            used_times[filename] = 0;
            std::string saved = filename;
            std::string attempt = filename;
            int fails = 1;
            while (output_names.find(attempt) != output_names.end())
            {
                std::stringstream out2;
                out2 << saved << "_" << fails;
                attempt = out2.str();
                fails++;
            }
            filename = attempt;
        }

        output_names.insert(filename);
        if (logging)
            printf("file: %s\n", filename.c_str());
        eparams.names.push_back(filename);
    }

    return true;
}

bool BrandonToolsApp::DoExportImages()
{
    if (eparams.images.size() > 1)
    {
        switch (eparams.mode)
        {
            case 0:
                printf("NOT IMPLEMENTED YET SILLY");
                return false;
            case 3:
                DoMode3Multi(eparams.images, eparams);
                break;
            case 4:
                DoMode4Multi(eparams.images, eparams);
                break;
            case 7:
                printf("NOT IMPLEMENTED YET SILLY");
                return false;
            default:
                printf("No mode specified image not exported");
                return false;
        }
    }
    else
    {
        switch (eparams.mode)
        {
            case 0:
                printf("NOT IMPLEMENTED YET SILLY");
                return false;
            case 3:
                DoMode3(eparams.images[0], eparams);
                break;
            case 4:
                DoMode4(eparams.images[0], eparams);
                break;
            case 7:
                printf("NOT IMPLEMENTED YET SILLY");
                return false;
            default:
                printf("No mode specified image not exported");
                return false;
        }
    }

    return true;
}

// Do cool things here
int BrandonToolsApp::OnRun()
{
    // Seed random number for quote generator the seed changes everyday
    // Well not exactly it will reset when GMT reaches 12am
    // Oh well close enough
    time_t timeh = time(NULL);
    srand(timeh / 60 / 60 / 24);

    cpercep_init();

    if (!Validate())
        return EXIT_FAILURE;
    if (!DoLoadImages())
        return EXIT_FAILURE;
    if (!DoHandleResize())
        return EXIT_FAILURE;
    if (!DoCheckAndLabelImages())
        return EXIT_FAILURE;
    if (!DoExportImages())
        return EXIT_FAILURE;

    printf("File exported successfully as %s.c and %s.h\n", eparams.name.c_str(), eparams.name.c_str());
    printf("The image (unless otherwise specified via command line) should be located in the current working directory (use ls and pwd)\n");

    return EXIT_SUCCESS;
}
