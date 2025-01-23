/*
 * Copyright (c) 2005, 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2022, Azul Systems, Inc. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef OS_POSIX_POSIXATTACHOPERATION_HPP
#define OS_POSIX_POSIXATTACHOPERATION_HPP

#include "services/attachListener.hpp"

class PosixAttachOperation;

#if INCLUDE_SERVICES

class SocketChannel : public AttachOperation::RequestReader, public AttachOperation::ReplyWriter {
private:
  int _socket;
public:
  SocketChannel(int socket) : _socket(socket) {}
  ~SocketChannel() {
    close();
  }

  bool opened() const {
    return _socket != -1;
  }

  void close() {
    if (opened()) {
      ::close(_socket);
      _socket = -1;
    }
  }

  // RequestReader
  int read(void* buffer, int size) override {
    ssize_t n;
    RESTARTABLE(::read(_socket, buffer, (size_t)size), n);
    return checked_cast<int>(n);
  }

  // ReplyWriter
  int write(const void* buffer, int size) override {
    ssize_t n;
    RESTARTABLE(::write(_socket, buffer, size), n);
    return checked_cast<int>(n);
  }
  // called after writing all data
  void flush() override {
    ::shutdown(_socket, SHUT_RDWR);
  }

  int write_fully(char* buf, size_t len) {
    do {
      ssize_t n = ::write(_socket, buf, len);
      if (n == -1) {
        if (errno != EINTR) return -1;
      } else {
        buf += n;
        len -= n;
      }
    }
    while (len > 0);
    return 0;
  }

  bool equals(int socket) {
    return socket == _socket;
  }
};

class PosixAttachOperation: public AttachOperation {
 private:
  // the connection to the client
  SocketChannel _socket_channel;
  bool _effectively_completed;
  void write_operation_result(jint result, bufferedStream* st);

 public:
  void complete(jint res, bufferedStream* st);
  void effectively_complete_raw(jint res, bufferedStream* st);
  bool is_effectively_completed()                      { return _effectively_completed; }

  PosixAttachOperation(int socket) : AttachOperation(), _socket_channel(socket) {
  }

  bool read_request() {
    return AttachOperation::read_request(&_socket_channel, &_socket_channel);
  }

  int write_fully(char* buf, size_t len) {
    return _socket_channel.write_fully(buf, len);
  }

  void close() {
    _socket_channel.close();
  }

  void shutdown() {
    _socket_channel.flush();
  }

  bool socket_equals(int socket) {
    return _socket_channel.equals(socket);
  }
};

#endif // INCLUDE_SERVICES

#endif // OS_POSIX_POSIXATTACHOPERATION_HPP
