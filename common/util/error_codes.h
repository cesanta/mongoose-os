// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Note: This is based on Google's util/task/codes.proto, but converted to
// simple enum to avoid pulling in protobufs as a dependency.

#pragma once

#include <string>

// Not an error; returned on success
#define STATUS_OK 0

// The operation was cancelled (typically by the caller).
#define STATUS_CANCELLED -101

// Unknown error.
#define STATUS_UNKNOWN -102

// Client specified an invalid argument.  Note that this differs
// from FAILED_PRECONDITION.  INVALID_ARGUMENT indicates arguments
// that are problematic regardless of the state of the system
// (e.g., a malformed file name).
#define STATUS_INVALID_ARGUMENT -103

// Deadline expired before operation could complete.
#define STATUS_DEADLINE_EXCEEDED -104

// Some requested entity (e.g., file or directory) was not found.
#define STATUS_NOT_FOUND -105

// Some entity that we attempted to create (e.g., file or directory)
// already exists.
#define STATUS_ALREADY_EXISTS -106

// The caller does not have permission to execute the specified
// operation.
#define STATUS_PERMISSION_DENIED -107

// Some resource has been exhausted, perhaps a per-user quota, or
// perhaps the entire file system is out of space.
#define STATUS_RESOURCE_EXHAUSTED -108

// Operation was rejected because the system is not in a state
// required for the operation's execution.  For example, directory
// to be deleted may be non-empty, an rmdir operation is applied to
// a non-directory, etc.
//
// A litmus test that may help a service implementor in deciding
// between FAILED_PRECONDITION, ABORTED, and UNAVAILABLE:
//  (a) Use UNAVAILABLE if the client can retry just the failing call.
//  (b) Use ABORTED if the client should retry at a higher-level
//      (e.g., restarting a read-modify-write sequence).
//  (c) Use FAILED_PRECONDITION if the client should not retry until
//      the system state has been explicitly fixed.  E.g., if an "rmdir"
//      fails because the directory is non-empty, FAILED_PRECONDITION
//      should be returned since the client should not retry unless
//      they have first fixed up the directory by deleting files from it.
#define STATUS_FAILED_PRECONDITION -109

// The operation was aborted, typically due to a concurrency issue
// like sequencer check failures, transaction aborts, etc.
//
// See litmus test above for deciding between FAILED_PRECONDITION,
// ABORTED, and UNAVAILABLE.
#define STATUS_ABORTED -110

// Operation was attempted past the valid range.  E.g., seeking or
// reading past end of file.
//
// Unlike INVALID_ARGUMENT, this error indicates a problem that may
// be fixed if the system state changes. For example, a 32-bit file
// system will generate INVALID_ARGUMENT if asked to read at an
// offset that is not in the range [0,2^32-1], but it will generate
// OUT_OF_RANGE if asked to read from an offset past the current
// file size.
#define STATUS_OUT_OF_RANGE -111

// Operation is not implemented or not supported/enabled in this service.
#define STATUS_UNIMPLEMENTED -112

// Internal errors.  Means some invariants expected by underlying
// system has been broken.  If you see one of these errors,
// something is very broken.
#define STATUS_INTERNAL -113

// The service is currently unavailable.  This is a most likely a
// transient condition and may be corrected by retrying with
// a backoff.
//
// See litmus test above for deciding between FAILED_PRECONDITION,
// ABORTED, and UNAVAILABLE.
#define STATUS_UNAVAILABLE -114

// Unrecoverable data loss or corruption.
#define STATUS_DATA_LOSS -115

std::string StatusToString(int error_code);
