/*
    uac -- bilexical compiler. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uac.

    uac is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uac.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */

#ifndef MODULEFILE_H
#define MODULEFILE_H

#include <vector>
#include <string>
#include <iostream>
#include "asm_keywords.h"
#include "parser_common.h"

using namespace std;

class ModuleFile
{
    vector<MetaSegment> metaSegments;
    vector<Segment> segments;
    string name, ext;
    byte flags;
    vector<byte> compiled_exe;
    vector<byte> compiled_funcs;
public:
    //ModuleFile() { }

    static const int NEW_SIGN = 0xDEADBABE;

    ModuleFile(string name, string ext) : name(name), ext(ext) {
        ;
    }

    void AddSegment(Segment seg) {
        for(auto iter = segments.begin(); iter != segments.end(); ++iter) {
            if(iter->GetName() == seg.GetName()) {
                throw string("Segment already present!");
                return;
            }
        }
        for(auto iter = metaSegments.begin(); iter != metaSegments.end(); ++iter) {
            if(iter->GetName() == seg.GetName()) {
                throw string("Segment already present!");
                return;
            }
        }
        segments.push_back(seg);
    }

    void AddSegment(MetaSegment seg) {
        for(auto iter = segments.begin(); iter != segments.end(); ++iter) {
            if(iter->GetName() == seg.GetName()) {
                throw string("Segment already present!");
                return;
            }
        }
        for(auto iter = metaSegments.begin(); iter != metaSegments.end(); ++iter) {
            if(iter->GetName() == seg.GetName()) {
                throw string("Segment already present!");
                return;
            }
        }
        metaSegments.push_back(seg);
    }

    void RemoveSegment(string name) {
        for(auto iter = segments.begin(); iter != segments.end(); ++iter) {
            if(iter->GetName() == name) {
                segments.erase(iter);
                return;
            }
        }
        for(auto iter = metaSegments.begin(); iter != metaSegments.end(); ++iter) {
            if(iter->GetName() == name) {
                metaSegments.erase(iter);
                return;
            }
        }
    }

    void SetFlags(byte flags) {
        this->flags = flags;
    }

    void Write() {
        int offset = 0;
        compiled_exe.push_back(0xDE);
        compiled_exe.push_back(0xAD);
        compiled_exe.push_back(0xBA);
        compiled_exe.push_back(0xBE);

        //push_int(0x10); //4x4
        //push_int(0x1);
        //push_int(0x1);
        //push_int(0x0);

        compiled_exe.push_back(flags);
        offset = 5;

        for(auto meta : metaSegments)
            if(meta.GetSize() != 0)
                offset += meta.GetName().size() + 1 + 1 + 4; // '\0' + type + addr

        for(auto seg : segments)
            if(seg.GetSize() != 0)
                offset += seg.GetName().size() + 1 + 1 + 4;

        auto lambda = [&](Segment* seg) {
            int sz = seg->GetSize();
            cout << "Size of segment " << seg->GetName() << " is " << sz << endl;
            if(sz != 0) {
                push_string(seg->GetName());
                compiled_exe.push_back(seg->GetFlags().val);
                push_int(offset);
                offset += sz;
            }
        };

        for(MetaSegment meta : metaSegments) {
            lambda(&meta);
        }
        for(Segment seg : segments) {
            lambda(&seg);
        }

        for(MetaSegment meta : metaSegments) {
            //cout << "Before getting metadata\n";
            auto& data = meta.GetData();
            compiled_exe.insert(compiled_exe.end(), data.begin(), data.end());
        }
        for(Segment seg : segments) {
            auto data = seg.GetData();
            compiled_exe.insert(compiled_exe.end(), data.begin(), data.end());
        }

        ofstream out(name + "." + ext);
        for(auto b : compiled_exe) {
            out.put(b);
        }
        out.close();
    }
private:
    void push_int(int num) {
        memory_unit u;
        u.word = num;
        //cout << "INT: " << u.word << endl;
        compiled_exe.push_back(u.bytes[0]);
        compiled_exe.push_back(u.bytes[1]);
        compiled_exe.push_back(u.bytes[2]);
        compiled_exe.push_back(u.bytes[3]);
    }

    void push_string(const string &str) {
        for(int i = 0; i < str.length(); i++)
            compiled_exe.push_back(str[i]);
        compiled_exe.push_back(0);
    }
};

#endif // MODULEFILE_H
