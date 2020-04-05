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

void buildXmlData(vector<string> &repValues, unsigned char *buffer) {
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_string((const char *) buffer);
    if (result) {
        unsigned int index = 0;
        for (auto tool : doc.child("E2AP-PDU")
                .child("initiatingMessage")
                .child("value")
                .child("E2setupRequest")
                .child("protocolIEs")
                .children("E2setupRequestIEs")) {
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

        streambuf *oldCout = cout.rdbuf();
        ostringstream memCout;
// create new cout
        cout.rdbuf(memCout.rdbuf());
        doc.save(std::cout);
//return to the normal cout
        cout.rdbuf(oldCout);
        memcpy(buffer, memCout.str().c_str(), memCout.str().length());
    }
}

#endif //E2_BUILDXML_H
