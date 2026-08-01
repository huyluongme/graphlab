#include "utils/project_serializer.h"
#include <fstream>
#include <cstdio>

namespace GraphLab::Utils {

    bool ProjectSerializer::SaveProject(
        const std::string& filepath,
        const std::vector<Math::Expression>& expressions,
        const std::unordered_map<std::string, UI::ParamState>& params,
        float zoom,
        ImVec2 panOffset
    ) {
        std::ofstream out(filepath);
        if (!out) return false;

        out << "# GraphLab Project File\n";
        out << "VIEWPORT " << zoom << " " << panOffset.x << " " << panOffset.y << "\n";

        out << "EXPRESSIONS_COUNT " << expressions.size() << "\n";
        for (const auto& exp : expressions) {
            out << "EXPR\n";
            out << "NAME " << exp.name << "\n";
            out << "VISIBLE " << exp.visible << "\n";
            out << "COLOR " << exp.color.x << " " << exp.color.y << " " << exp.color.z << " " << exp.color.w << "\n";
            out << "SHOW_LABEL " << exp.showLabel << "\n";
            out << "LABEL_TEXT " << (exp.labelText[0] ? exp.labelText : "_") << "\n";
            out << "CONNECT_LINE " << exp.connectLine << "\n";
        }

        out << "PARAMS_COUNT " << params.size() << "\n";
        for (const auto& [pName, state] : params) {
            out << "PARAM " << pName << " " << state.value << " " << state.minVal << " " << state.maxVal << "\n";
        }

        out.close();
        return true;
    }

    bool ProjectSerializer::LoadProject(
        const std::string& filepath,
        std::vector<Math::Expression>& expressions,
        std::unordered_map<std::string, UI::ParamState>& params,
        float& outZoom,
        ImVec2& outPanOffset
    ) {
        std::ifstream in(filepath);
        if (!in) return false;

        expressions.clear();
        params.clear();

        std::string tag;
        while (in >> tag) {
            if (tag == "VIEWPORT") {
                in >> outZoom >> outPanOffset.x >> outPanOffset.y;
            } else if (tag == "EXPR") {
                Math::Expression exp;
                std::string subTag;
                while (in >> subTag) {
                    if (subTag == "NAME") {
                        in.get(); // skip space
                        std::string line;
                        std::getline(in, line);
                        snprintf(exp.name, sizeof(exp.name), "%s", line.c_str());
                    } else if (subTag == "VISIBLE") {
                        in >> exp.visible;
                    } else if (subTag == "COLOR") {
                        in >> exp.color.x >> exp.color.y >> exp.color.z >> exp.color.w;
                    } else if (subTag == "SHOW_LABEL") {
                        in >> exp.showLabel;
                    } else if (subTag == "LABEL_TEXT") {
                        std::string lbl;
                        in >> lbl;
                        if (lbl != "_") snprintf(exp.labelText, sizeof(exp.labelText), "%s", lbl.c_str());
                    } else if (subTag == "CONNECT_LINE") {
                        in >> exp.connectLine;
                        break;
                    }
                }
                exp.Recompile();
                expressions.push_back(exp);
            } else if (tag == "PARAM") {
                std::string pName;
                UI::ParamState state;
                in >> pName >> state.value >> state.minVal >> state.maxVal;
                params[pName] = state;
            }
        }

        in.close();
        return true;
    }

} // namespace GraphLab::Utils
