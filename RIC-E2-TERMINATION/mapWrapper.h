/*
 * Copyright 2019 AT&T Intellectual Property
 * Copyright 2019 Nokia
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
// Created by adi ENZEL on 8/28/19.
//

#ifndef E2_MAPWRAPPER_H
#define E2_MAPWRAPPER_H

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <string>
#include <iostream>

using namespace std;

class mapWrapper {
public:
    void *find(char *key) {
        shared_lock<shared_timed_mutex> read(fence);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {

        }
        auto entry = keyMap.find(key);
        if (entry == keyMap.end()) {
            return nullptr;
        }
        return entry->second;
    }

    void setkey(char *key, void *val) {
        unique_lock<shared_timed_mutex> write(fence);
        keyMap[key] = val;
    }

    void *erase(char *key) {
        unique_lock<shared_timed_mutex> write(fence);
        return (void *)keyMap.erase(key);
    }

    void clear() {
        unique_lock<shared_timed_mutex> write(fence);
        keyMap.clear();
    }

    void getKeys(vector<char *> &v) {
        shared_lock<shared_timed_mutex> read(fence);
        for (auto const &e : keyMap) {
            v.emplace_back((char *)e.first.c_str());
        }
    }



private:
    std::unordered_map<string, void *> keyMap;
    shared_timed_mutex fence;

};
#endif //E2_MAPWRAPPER_H
