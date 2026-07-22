# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## unreleased

### Changed

 - Reply with `400 Bad Request` when an incoming request fails to parse, instead of silently draining the buffer and leaving the peer to hang until timeout. Follow-up to [OpenIPC/smolrtsp#59](https://github.com/OpenIPC/smolrtsp/pull/59).

## 0.1.0 - 2022-03-31

### Added

 - This awesome library.
