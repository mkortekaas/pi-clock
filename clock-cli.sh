#!/bin/bash

# Run pi-clock with config from YAML file.
# All settings (matrix hardware, display, cities, temperature) live in config.yaml.
# Pass -c to use a different config file.

./pi-clock -c config.yaml
