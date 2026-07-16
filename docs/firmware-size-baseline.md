# Firmware size baseline

Recorded before the configuration/domain migration on 2026-07-13 with
PlatformIO environment `wemos_d1_mini32`:

| Resource | Used | Available | Percentage |
| --- | ---: | ---: | ---: |
| Static RAM | 117,160 bytes | 327,680 bytes | 35.8% |
| Flash | 986,381 bytes | 1,310,720 bytes | 75.3% |

The first milestone build after the migration used 117,040 bytes of RAM and
985,853 bytes of flash. These figures are build-time size reports; runtime heap
headroom should be measured separately on hardware.

After the Phase 3 service decomposition, the build used 117,048 bytes of RAM
and 986,117 bytes of flash.

After packaging the services as Phase 4 PlatformIO libraries, the build used
117,104 bytes of RAM and 987,941 bytes of flash.

After the Phase 5 feature/application composition, the build used 117,172 bytes
of RAM and 988,241 bytes of flash.
