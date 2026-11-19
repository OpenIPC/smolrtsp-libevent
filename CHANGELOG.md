# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## unreleased

### Fixed

 - A request still incomplete after `SMOLRTSP_LIBEVENT_MAX_REQUEST` bytes (128 KiB, overridable) is dropped and the connection is handed to the event callback as a `BEV_EVENT_READING | BEV_EVENT_ERROR`, so integrators close it through their usual read-error path. Before, a request that never completed -- a header with no CRLF, a URI with no space, a huge `Content-Length` -- was buffered without limit and re-parsed from its first byte on every read, so one unauthenticated connection could starve the event loop and then exhaust memory.

### Changed

 - Reply with `400 Bad Request` when an incoming request fails to parse, instead of silently draining the buffer and leaving the peer to hang until timeout. Follow-up to [OpenIPC/smolrtsp#59](https://github.com/OpenIPC/smolrtsp/pull/59).

## 0.1.0 - 2022-03-31

### Added

 - This awesome library.
