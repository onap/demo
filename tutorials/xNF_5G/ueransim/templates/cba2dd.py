#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#   COPYRIGHT NOTICE STARTS HERE

#   Copyright 2020 . Samsung Electronics Co., Ltd.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

#   COPYRIGHT NOTICE ENDS HERE

import os
import argparse
import json

#
# Script to convert given Enriched CDS CBA model to Data Dictionary output
# Usage:
#   ./cba2dd.py --cba_dir <path to cba main directory> | python3 -m json.tool
#

def get_resources_definition_file(cba_dir):
    definitions_dir = cba_dir + os.sep + "Definitions"
    resource_definition_file = definitions_dir + os.sep + "resources_definition_types.json"
    if not os.path.exists(definitions_dir):
        raise RuntimeError("'%s' directory does not exists or is not CBA directory" % cba_dir)
    if not os.path.exists(resource_definition_file):
        raise RuntimeError("'%s' file does not exists in CBA Definitions directory. CBA is not Enriched!" % resource_definition_file)
    return resource_definition_file

def create_dd(cba_dir):
    with open(get_resources_definition_file(cba_dir)) as f:
        output_json = json.load(f)
    dd = []
    for _, entry in output_json.items():
        dd.append(build_dd_entry(entry))
    print(json.dumps(dd))

def build_dd_entry(definition_entry):
    """Builds Data Dictionary entry from given dictionary entry. Given entry
    itself is added as value for "definition" key.
    {
        "name": "",
        "tags": "",
        "data_type": "",
        "description": "",
        "entry_schema": "",
        "updatedBy": "",
        "definition": definition_entry
    }
    """
    out_dict = {}
    out_dict["name"] = definition_entry["name"]
    out_dict["tags"] = definition_entry["tags"]
    out_dict["data_type"] = definition_entry["property"]["type"]
    out_dict["description"] = definition_entry["property"]["description"]
    out_dict["entry_schema"] = definition_entry["property"]["type"]
    out_dict["updatedBy"] = definition_entry["updated-by"]
    out_dict["definition"] = definition_entry
    return out_dict

def main():
    description = """Script to convert given Enriched CDS CBA model to Data Dictionary output.
Example:
  ./cba2dd.py --cba_dir cba | python3 -m json.tool
    """
    parser = argparse.ArgumentParser(description=description,
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--cba_dir',
                        help='Path to CDS CBA model main directory',
                        default='')
    args = parser.parse_args()
    try:
        create_dd(args.cba_dir)
    except Exception as e:
        print(e)
        parser.print_help()
        exit(1)

if __name__ == '__main__':
    main()
