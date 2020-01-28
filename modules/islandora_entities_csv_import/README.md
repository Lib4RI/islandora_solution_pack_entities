# Islandora Entities CSV Import [![Build Status](https://travis-ci.org/Islandora/islandora_solution_pack_entities.png?branch=7.x)](https://travis-ci.org/Islandora/islandora_solution_pack_entities)

## Introduction

This module is for adding person entities to Islandora using a .csv file. 

## Requirements

This module requires the following modules/libraries:
* [Islandora](https://github.com/islandora/islandora)
* [Islandora Basic Collection](https://github.com/Islandora/islandora_solution_pack_collection)
* [Islandora Entities](https://github.com/Islandora/islandora_solution_pack_entities)

## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

Prepare a 'pipe'-delimited CSV ('pipe' means vertical bar) using the column names below,
however you may re-configure this in admin/islandora/solution_pack_config/entities

Multiple arguments within one CSV column can be separated with a tilde (~). However, this may yield unexpected results 
(missing XML attributes, improper nesting) if used outside the following fields: FAX, PHONE, EMAIL, POSITION. 

Comma within a field/cell (example: 'Trump, Donald') are supported now.

## Formal Background

The fields used in the CSV may not match the [Outline of Elements and Attributes in MADS](http://www.loc.gov/standards/mads/mads-outline-2-0.html) directly,
but they will be mapped accoording to this outline when the MADS XML file will be created.

## CSV Restriction

Only columns with names in the list will be processed; all others will be ignored.
```
STATUS		/* will become element <note type='status'> in MADS */
POSITION
EMAIL
BUILDING
ROOM_NUMBER
IDENTIFIER
TERM_OF_ADDRESS
GIVEN_NAME
FAMILY_NAME
FAX
PHONE
DISPLAY_NAME
DEPARTMENT	/* will become element <organization> in MADS */
BUILDING
CAMPUS
NAME_DATE
STREET
CITY
STATE
COUNTRY
POSTCODE
HOURS
START_DATE
END_DATE
ROOM_NUMBER
BUILDING
CAMPUS
```

This will be transformed into the following MADS record:

```xml
<mads xmlns="http://www.loc.gov/mads/v2" xmlns:mads="http://www.loc.gov/mads/v2"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink">
    <authority>
        <name type="personal">
            <namePart type="given">[GIVEN_NAME]</namePart>
            <namePart type="family">[FAMILY_NAME]</namePart>
            <namePart type="termsOfAddress">[TERM_OF_ADDRESS]</namePart>
            <namePart type="date">[NAME_DATE]</namePart>
        </name>
        <titleInfo>
            <title>[DISPLAY_NAME]</title>
        </titleInfo>
    </authority>
    <affiliation>
        <organization>[DEPARTMENT]</organization>
        <position>[POSITION]</position>
		<email>[EMAIL]</email>
		<phone>[PHONE]</phone>
		<fax>[FAX]</fax>
        <address>
		    <street>[STREET]</street>
		    <city>[CITY]</city>
		    <state>[STATE]</state>
		    <country>[COUNTRY]</country>
		    <postcode>[POSTCODE]</postcode>
	    </address>
		<dateValid point='start'>[START_DATE]</dateValid>
		<dateValid point='end'>[END_DATE]</dateValid>
    </affiliation>
    <note type="address">[ROOM_NUMBER] [BUILDING] [CAMPUS]</note>
    <identifier type="u1">[IDENTIFIER]</identifier>
    <note type="status">[STATUS]</note>
</mads>
```

## Documentation

Further documentation for this module is available at [our wiki](https://wiki.duraspace.org/display/ISLANDORA/Entities+Solution+Pack).

## Troubleshooting/Issues

Having problems or solved a problem? Check out the Islandora google groups for a solution.

* [Islandora Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora)
* [Islandora Dev Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora-dev)

## Maintainers/Sponsors

Current maintainers:

* [Rosie Le Faive](https://github.com/rosiel)

## Development

If you would like to contribute to this module, please check out our helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the Islandora.ca site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
