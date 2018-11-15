
## 1.2.3 (2018-11)

### Added
- distributed as a debian package, more convenient installation
- adapt to librespeaker v2.1.1+
- configuration can also be done via configuration file at /etc/respeaker/respeakerd.conf
- support for snips.ai hotword engine

### Changed
- pulse mode: if the pipe file hasn't been created, respeakerd will wait, rather just quit in the previous version
- replace gflags with gnu getopt

### Removed
- remove hotword engine model files, now these resource files are released with librespeaker
