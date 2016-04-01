## Introduction
laksjdf;laksdfj
ICG CSV Import Module

ICG Main site:  https://sites.google.com/site/islandoraconsortiagroup/home


## Requirements

This module requires the following modules/libraries:

* [Tuque](https://github.com/islandora/tuque)

Tuque is expected to be in one of two paths:

* sites/all/libraries/tuque (libraries directory may need to be created)
* islandora_folder/libraries/tuque

## Installation

Install it as any other Islandora Drupal module in sites/all/modules.

## Configuration

[Don't know if any Drupal configure menu will be necessary.]

## Documentation

The CSV file needs to be comma separate with quotation marks around all values. For sample CSV files see the "examples" directory. 

The CSV import form prompts for
* Content Model (all items need to use the same content model)
* Metadata schema (MODS, other may be added later)
* namespace (all items need to use the same namespace)
* file to import (the CSV file)
** If the file uploaded in not valid, it will not be ingestable.
* field-by-field mapping of the CSV file to MODS elements.

## Troubleshooting/Issues

Having problems or solved a problem? Check out the Islandora google groups for a solution.

* [Islandora Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora)
* [Islandora Dev Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora-dev)

## Maintainers/Sponsors

Current maintainers:

https://sites.google.com/site/islandoraconsortiagroup/home

http://commonmedia.com

## Development

If you would like to contribute to this module, please check out [CONTRIBUTING.md](CONTRIBUTING.md). 

In addition, there is helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as the [Developers](http://islandora.ca/developers) section on the [Islandora.ca](http://islandora.ca) site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)


