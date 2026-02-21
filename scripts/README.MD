# Scripts
___

Scripts collection for utility functions

Entrypoint scripts:
1. [make_examples.sh](make_examples.sh) - Script to generate regression fixtures (bytecode, inputs, outputs).
2. [make_perf.sh](make_perf.sh) - Script to generate performance fixtures (bytecode only).
3. [clean_temp.sh](clean_temp.sh) - Script to clean up temporary files created.
4. [docker_setup.sh](docker_setup.sh) - Script to set up Docker environment for the project.

Helper scripts:
1. [make_suite.sh](make_suite.sh) - Shared helper to build and copy fixtures for a suite.
2. [docker_lama.sh](docker_lama.sh) - Example script to run Docker containers for LaMa.
3. [build_examples.sh](build_examples.sh) - Script to build regression fixtures using specified parameters.
