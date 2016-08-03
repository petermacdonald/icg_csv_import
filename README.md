## Introduction

ICG CSV Import Module

This module is designed to ingest a single CSV file containing one or more object records, with each record representing 
the MODS metadata and related information required to create one Islandora/Fedora object.

-All bugs, feature requests and improvement suggestions are tracked at the [DuraSpace JIRA](https://jira.duraspace.org/browse/ISLANDORA).-


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

The input data file, or CSV file, may employ any reasonable field seperator.  Commas are most often used, hence the term comma-seperated-values or CSV, 
but other delimiters like tabs or semi-colons may be used.  A 'pipe' or vertical bar delimiters are used to seperate multiple values **within** fields, so pipes should NOT be used to delimit the fields themselves.  Values should be enclosed in quotation marks (double quotes, not single). For sample CSV files see the "examples" directory. 
 
The batch process used to import CSV data is file-driven, with most of the necessary input stored directly in the CSV data file.  The module's user interface may be employed to assist with building a suitably structured CSV file.  

### CSV File Structure

As indicated above, this module relies on a suitably structured CSV file to drive its batch processing, but the structure of can be relatively flexible.  A small sample of CSV data from a fossils collection in the Grinnell College Geology Collection is used, below, to illustrate features of the CSV file structure.  These samples are presented as annotated screen grabs of the data as it appears in Excel, but any means of editing/preparing CSV data may be employed.

The initial/raw CSV data for our example looks like this in Excel:

![Raw CSV Data](documentation/images/Fossils-01.png?raw=true)

![CSV Data with Import Index Added](../documentation/images/Fossils-02.png?raw=true)







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

* [Mark McFate](https://github.com/McFateM)
* [Steve Young](https://github.com/hamhpc)
* [Peter MacDonald](https://github.com/petermacdonald)
* [Jessika Drmacich](https:/github.com/jgd1)

## Development

If you would like to contribute to this module, please check out [CONTRIBUTING.md](CONTRIBUTING.md). In addition, we have helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the [Islandora.ca](http://islandora.ca) site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)





