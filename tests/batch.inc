<?php

 function icg_csv_import_test() {
    $xml_form_id = NULL;
    $parent_object = NULL;
    $namespace = "test";
    $headers = array( "/mods:mods/mods:identifier[@type='local']","/mods:mods/mods:titleInfo/mods:title","/mods:mods/mods:name[@type='personal']/mods:role/mods:roleTerm[@type='text' and text()='creator']/../../mods:namePart","/mods:mods/mods:originInfo/mods:dateCreated" );

    // There are supposed to be four fields in the CSV file ("Identifier","Title","Author","Date")
    // Some of the CSV rows have more than four fields (such as the first row, which has "Hannah" and "Adams" for the author)

    $row = array( "1794 1","A view of religions, in two parts. : Part I. Containing an alphabetical compendium of the various religious denominations, which have appeared in the world, from the beginning of the Christian era to the present day. Part II. Containing a brief account of the different schemes of religion now embraced among mankind. The whole collected from the best authors, ancient and modern","Hannah","Adams","1791");
    $pid = icg_csv_import_batch_item($headers, $row, $xml_form_id, $parent_object, $namespace);

    // same thing here with author field: "Benjamin" and "Fawcett"
    $row = array( "1794 2","The saint's everlasting rest, or, A treatise of the blessed state of the saints in their enjoyment of God in heaven. Written by the reverend, learned, and pious Mr. Richard Baxter","Benjamin","Fawcett","1793");
    $pid = icg_csv_import_batch_item($headers, $row, $xml_form_id, $parent_object, $namespace);

    // this row seems corrupt; there is no author but the author field seems to contain info from the title field
    $row = array( "1794 3","Evidences of the Christian religion; briefly and plainly","stated","1787");
    $pid = icg_csv_import_batch_item($headers, $row, $xml_form_id, $parent_object, $namespace);

    $row = array( "1794 4","Bible","","1801");
    $pid = icg_csv_import_batch_item($headers, $row, $xml_form_id, $parent_object, $namespace);
  }

}
