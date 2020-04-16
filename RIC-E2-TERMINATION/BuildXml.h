/*
 * Copyright 2020 AT&T Intellectual Property
 * Copyright 2020 Nokia
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by adi ENZEL on 4/5/20.
//

#ifndef E2_BUILDXML_H
#define E2_BUILDXML_H
#include <iostream>
#include <iosfwd>
#include <vector>
#include "pugixml/src/pugixml.hpp"
#include <string.h>
#include <sstream>

using namespace std;

/*
 * Copied from pugixml samples
 */
struct xml_string_writer : pugi::xml_writer {
    std::string result;

    virtual void write(const void *data, size_t size) {
        result.append(static_cast<const char *>(data), size);
    }
};
// end::code[]

struct xml_memory_writer : pugi::xml_writer {
    char *buffer;
    size_t capacity;
    size_t result;

    xml_memory_writer() : buffer(0), capacity(0), result(0) {
    }

    xml_memory_writer(char *buffer, size_t capacity) : buffer(buffer), capacity(capacity), result(0) {
    }

    size_t written_size() const {
        return result < capacity ? result : capacity;
    }

    virtual void write(const void *data, size_t size) {
        if (result < capacity) {
            size_t chunk = (capacity - result < size) ? capacity - result : size;

            memcpy(buffer + result, data, chunk);
        }
        result += size;
    }
};

std::string node_to_string(pugi::xml_node node) {
    xml_string_writer writer;
    node.print(writer);

    return writer.result;
}


void buildXmlData(const string &messageName, const string &ieName, vector<string> &repValues, unsigned char *buffer) {
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_string((const char *)buffer);
    if (result) {
        unsigned int index = 0;
        for (auto tool : doc.child("E2AP-PDU")
                .child("initiatingMessage")
                .child("value")
                .child(messageName.c_str())
                .child("protocolIEs")
                .children(ieName.c_str())) {
            for (auto n : tool.child("value").child("RANfunctions-List").child(
                    "ProtocolIE-SingleContainer").children()) {
//ProtocolIE-SingleContainer
//cout << "\t1 " << n.name() << endl;
                if (strcmp(n.name(), "value") == 0) {
                    for (auto l : tool.child("value").children()) {
//cout << "\t\t2 " << l.name() << endl;
                        for (auto f : l.children()) {
//cout << "\t\t\t3 " << f.name() << endl;
                            for (auto g : f.child("value").children()) {
//cout << "\t\t\t\t4 " << g.name() << endl;
                                for (auto a : g.children()) {
                                    if (strcmp(a.name(), "ranFunctionDefinition") == 0) {
                                        if (repValues.size() > index) {
                                            a.remove_children();
                                            string val = repValues.at(index++);
// here we get vector with counter
                                            a.append_child(pugi::node_pcdata).set_value(val.c_str());

                                        }
                                    }
//cout << "\t\t\t\t\t5 " << a.name() << " " << a.child_value() << endl;
                                }
                            }
                        }
                    }
                }
            }
        }

        auto res = node_to_string(doc);
        memcpy(buffer, res.c_str(), res.length());

//        streambuf *oldCout = cout.rdbuf();
//        ostringstream memCout;
//// create new cout
//        cout.rdbuf(memCout.rdbuf());
//        doc.save(std::cout);
////return to the normal cout
//        cout.rdbuf(oldCout);
//        memcpy(buffer, memCout.str().c_str(), memCout.str().length());
    }
}

#endif //E2_BUILDXML_H
