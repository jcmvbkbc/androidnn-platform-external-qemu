#!/bin/sh

# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/option_parser.shi

PROGRAM_DESCRIPTION=\
"Generates the QEMU1 sources

You usually want to run this script after you have merged in new changes from the qemu
branch.

Since we are no longer actively developing QEMU1, you should not ever have to run this
script.
"

aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option

# Generate the QAPI headers and sources from qapi-schema.json
# Ideally, this would be done in our Makefiles, but as far as I
# understand, the platform build doesn't support a single tool
# that generates several sources files, nor the standalone one.
export PYTHONDONTWRITEBYTECODE=1
PROGDIR=${AOSP_DIR}/external/qemu/android
AUTOGENERATED_DIR=${PROGDIR}/qemu1/qemu1-qapi-auto-generated

log "Generating: $AUTOGENERATED_DIR"
QAPI_SCHEMA=$PROGDIR/qemu1/qapi-schema.json
QAPI_SCRIPTS=$PROGDIR/qemu1/scripts
make_if_not_exists "$AUTOGENERATED_DIR"
run python $QAPI_SCRIPTS/qapi-types.py qapi.types --output-dir=$AUTOGENERATED_DIR -b < $QAPI_SCHEMA
run python $QAPI_SCRIPTS/qapi-visit.py --output-dir=$AUTOGENERATED_DIR -b < $QAPI_SCHEMA
run python $QAPI_SCRIPTS/qapi-commands.py --output-dir=$AUTOGENERATED_DIR -m < $QAPI_SCHEMA


