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

In the preceeding image, the row of data highlighted in blue contains the CSV data 'headers' or descriptions, while all other rows contain data to be imported.  Note that some cells are empty and some cells contain commas within.  Both empty cells and embedded commas are permitted, but if commas are present in any data field, care should be taken to save the file with tab or other non-comma delimiters.  Or, if data is exported in a true CSV file, care should be taken to ensure that all values are enclosed in double quotes. 

It's good practice to introduce an additional column into your CSV data as illustrated below.

![CSV Data with 'Import Index' Added](documentation/images/Fossils-02.png?raw=true)

In the preceeding image, the column of data highlighted in blue has been prepended to the CSV data.  This column is optional and is intended to hold a simple sequence number for each row of data.  If the data is sorted while preparring for import, this column can be used to easily restore the original order.  The header, "# Import Index" in this colunn is also significant because it begins with a hashtag (or pound) symbol.  

#### Hashtag Identifies a Comment

Two rules govern the use of the hashtag (#) character to identify comments in an import CSV file:  

* A hashtag as the **first character** in any value/cell identifies that value as a comment. 
* A hastag as the **first character of the first cell in a row** identifies the entire row as a comment. 

Comments in the CSV data are read and recorded during processing, but they are otherwise ignored.

The introduction of "# Import Index" column, and the hashtag as the first character in the first cell of the headers row effectively renders all of the headers as comments.  This is as it should be, because we don't want the system to import the headers!

#### XPaths and Keys

The import process performed by this module is driven by simple XPath 1.0 statements and Keys or reserved keywords.  These XPath statements and Keys must appear in the **first row** of the CSV file as shown in the image below.

![CSV Data with XPaths](documentation/images/Fossils-03.png?raw=true)

The first row, highlighted in blue in the above image, contains a set of XPath statments and one Key (or keyword). 

##### XPaths

In the first column, the XPath is "/mods/note[@displayLabel='Import Index']".  This Xpath statement directs the module to import the corresponding column of data into a series of MODS datastreams.  The first row of data would generated the following MODS structure:

	<mods>
		<note displayLabel='Import Index>1</note>
	</mods> 
	
The second column in this example contains the Key or keyword "OBJ".  Keys are discussed in the next section of this document.

All of the remaining columns in our example contain XPath statements in row one. These XPaths, including the aforementioned statement at the top of the first colunm, will produce a MODS datastream for the first row of data as follows:

	<mods>
		<note displayLabel='Import Index>1</note>
		<identifier type='local'>EM-07-01</identifier>
		<titleInfo><title>Neuropteris hirsuta</title></titleInfo>
		<subject>
			<temporal>Pensylvanian, Upper Carboniferous Francis Creek Shale</temporal>
			<geographic>Mazon Creek, Grundy Co., Ill., Coal Measures</geographic>
		</subject>
	</mods> 
	
##### XPath Rules

The XPath 1.0 specification includes many useful tools, but the CSV Import module is restricted to using very simple XPath statements as outlined in the following rules.

* One Attribute per Element - Attributes, like type='local' in the identifier column of our example, are limited to **not more than one per element**.  Under this rule the following XPaths are valid because each element has only zero or one attributes:

	* /mods/titleInfo/title
	* /mods/name/namePart[@type='given']
	* /mods/name[@type='personal']/namePart[@type='given']     
	
	The following XPath statements are invalid because they contain elements with more than one attribute:

	* /mods/name[@type='personal'][@displayLabel='given']     
	* /mods/name/namePart[@type='personal'][@displayLabel='given'] 

* Elements can be Indexed - Some data constructs require the use of element indicies.  Consider this common MODS construct:

		<mods>
	   		<name type='personal'>
	   			<namePart>Doe, John</namePart>
	   			<role>
	   				<roleTerm type='text'>Gentleman</roleTerm>
	   			</role>
	   		</name>
	   		<name type='personal'>
	   			<namePart>Doe, Jane</namePart>
	   			<role>
	   				<roleTerm type='text'>Lady</roleTerm>
	   			</role>
	   		</name>
	   		<name type='corporate'>
	   			<namePart>Widget Corp.</namePart>
	   			<role>
	   				<roleTerm> type='text'>Sponsor</roleTerm>
	   			</role>
	   		</name> 
	   	</mods>	
		
	The XPaths and corresponding data (afer the arrow -->) look like this:
	
		/mods/name[1][@type='personal']
			/mods/name[1]/namePart  --> Doe, John
			/mods/name[1]/role/roleTerm[@type='text']  --> Gentleman
		/mods/name[2][@type='personal']
			/mods/name[2]/namePart  --> Doe, Jane
			/mods/name[2]/role/roleTerm[@type='text']  --> Lady
		/mods/name[3][@type='corporate']
			/mods/name[3]/namePart  --> Widget Corp.
			/mods/name[3]/role/roleTerm[@type='text']  --> Sponsor
			
	In this example, the indices [1], [2], and [3] are needed because there are three distinct name elements here, and the indicies ensure that the corresponding role elements are assinged to the correct names.	
	
	Indicies always begin with [1], never [0].  An index should always preceed the attribute (if one is present) attached to an element.  An index is declared when it is first encountered on an element, and indicies must always be 'declared' in numeric order, but may be referenced in any order.  For example, the following is a valid set of indexed elements:
	
		/mods/name[1], /mods/name[2], /mods/name[3], /mods/name[2]/role, /mods/name[1]/role
		
	This is a valid sequnce of XPaths because name[1] is declared before /name[2], which is declared before /name[3].  /mods/name[2]/role appears before /mods/name[1]/role but this is permitted since these are not declarations of the indicies, they are referenes to indicies which have already been declared.

	An invalid sequence of XPaths might look like this:

		/mods/name[1], /mods/name[3]/role, /mods/name[2]/role

	Note that in the line above the declaration of name[3] appears before the declaration of name[2] and this may create errant imported objects.
	
##### Keys

Keys, or keywords, like "OBJ" in the second column of our example, are always expressed in uppercase.  OBJ is the keyword that informs the system to read a filename from the corresponding data, and the file identifed by OBJ may be subsequently downloaded and saved as the object's content datastream with a datastream ID of OBJ.

The complete set of Keys and their corresponding behavior in the system are documented here.

* OBJ - The filename, or complete network path, to an object's OBJ or content datastream.
* OBJ_PREFIX - A prefix, typically the network path to a directory containing the files named in an OBJ column.
* CMODEL - The Islandora content model of the imported object.  Examples may include "islandora:sp_basic_image", "islandora:compoundCModel", etc.
* LABEL - Indicates that corresponding data is to be used as the object lable or title.




### CSV Import User Interface

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











