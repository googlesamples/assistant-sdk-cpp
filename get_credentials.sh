#!/bin/bash

#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# The input of this file is client_secret json file. Please download it to this folder and rename it to client_secret.json
# If succeeded, it will output credentials.json, which can be used with run_assistant.

CLIENT_SECRET_FILE="client_secret.json"
if ! [ -f $CLIENT_SECRET_FILE ] ; then
  echo "Please download client_secret json file to this folder, and change its name to '$CLIENT_SECRET_FILE'."
  exit 1
fi

CLIENT_ID=`cat $CLIENT_SECRET_FILE | grep -o -e '"client_id":"[^"]*"' | sed -e 's/.*"client_id":"\([^"]*\)".*/\1/g'`
if [ -z $CLIENT_ID ]; then
  echo "No client_id found in client secret file $CLIENT_SECRET_FILE. Exiting."
  exit 1
fi
echo "Client id: $CLIENT_ID"
CLIENT_SECRET=`cat $CLIENT_SECRET_FILE | grep -o -e '"client_secret":"[^"]*"' | sed -e 's/.*"client_secret":"\([^"]*\)".*/\1/g'`
if [ -z $CLIENT_SECRET ]; then
  echo "No client_secret found in client secret file $CLIENT_SECRET_FILE. Exiting."
  exit 1
fi
echo "Client secret: $CLIENT_SECRET"
echo ""

echo "Please go to the following link to authorize, and copy back the code here:"
echo "https://accounts.google.com/o/oauth2/v2/auth?scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fassistant-sdk-prototype&access_type=offline&include_granted_scopes=true&state=state&redirect_uri=urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob&response_type=code&client_id=$CLIENT_ID"
echo ""
echo -n "Code: "
read CODE

CREDENTIALS_FILE="credentials.json"
curl -s -X POST -d 'code='$CODE'&client_id='$CLIENT_ID'&client_secret='$CLIENT_SECRET'&redirect_uri=urn:ietf:wg:oauth:2.0:oob&grant_type=authorization_code' https://www.googleapis.com/oauth2/v4/token -o $CREDENTIALS_FILE
REFRESH_TOKEN=`cat $CREDENTIALS_FILE | grep -o -e '"refresh_token": "[^"]*"' | sed -e 's/"refresh_token": "\([^"]*\)"/\1/g'`
if [ -z $REFRESH_TOKEN ]; then
  echo "Failed to get refresh token. Exiting. OAuth server response:"
  cat $CREDENTIALS_FILE
  exit 1
fi

cat <<EOF > $CREDENTIALS_FILE
{
  "client_id": "$CLIENT_ID",
  "client_secret": "$CLIENT_SECRET",
  "refresh_token": "$REFRESH_TOKEN",
  "type": "authorized_user"
}
EOF
echo ""
echo "$CREDENTIALS_FILE is ready for use :-)"
