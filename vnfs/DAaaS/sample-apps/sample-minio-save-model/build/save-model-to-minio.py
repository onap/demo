#!/usr/bin/env python
#
# -------------------------------------------------------------------------
#   Copyright (c) 2019 Intel Corporation Intellectual Property
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
#
# -------------------------------------------------------------------------
#

'''Sample Python application to save model to Minio'''

# python imports

import json
import os
import time
from minio import Minio
from minio.error import ResponseError


client = Minio(os.environ['S3_ENDPOINT'],
               access_key=os.environ['AWS_ACCESS_KEY_ID'],
               secret_key=os.environ['AWS_SECRET_ACCESS_KEY'],
               secure=False)


def load_model(bucket_name, object_name, filepath):    
    try:
        client.fget_object(bucket_name, object_name, filepath)
    except ResponseError as err:
        print(err)

if __name__ == "__main__":
    bucket_name = os.environ['MODEL_BUCKET']
    location = os.environ['S3_REGION']
    model = os.environ['MODEL_FILE']
    filepath = "/app/" + model
    if not client.bucket_exists(bucket_name):
        client.make_bucket(bucket_name, location=location)
    
    found = False
    try:
        client.stat_object(bucket_name, model);
        found = True
    except Exception as err:
        found = False
    
    metadata = {
        "X-Amz-Meta-model-description": "Sample numpy model",
        "X-Amz-Meta-model-type": "scikit-learn/numpy",
        "X-Amz-Meta-model-version": "v1.0.0",
    }
    if not found:
        try:
            client.fput_object(bucket_name, model, filepath, metadata=metadata)
        except expression as identifier:
            print(err)
        
