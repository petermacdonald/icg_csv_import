<?php
/**
 * Created by PhpStorm.
 * User: markmcfate
 * Date: 4/8/16
 * Time: 1:38 PM
 * 
 * This include file provides a single, all-inclusive function, icg_csv_import_test( ) with no
 * arguments.  The function should return TRUE if passing, FALSE if not. 
 * In lieu of using Drupal 7's Testing (Simpletest) framework you can
 * customize this code and call it simply like ...$passed = icg_csv_import_test();.
 * 
 */

function icg_csv_import_test() {
  if (!$xp = file_get_contents('/var/www/drupal/sites/all/modules/icg_csv_import/templates/MODS.xml')) {
    drupal_set_message("Could not open 'MODS.xml'.", 'error');
    return FALSE;
  }
  $xml = new DOMDocument( );
  if (!$xml->loadXML($xp)) {
    drupal_set_message("Could not load 'MODS.xml' as a DOMDocument.", 'error');
    return FALSE;
  }
  $xpaths = array(
    "/mods/identifier[@type='local']",
    "/mods/titleInfo[1]/title",
    "/mods/titleInfo[2][@type='alternative']/title",
    "/mods/abstract",
    "/mods/name[1][@type='personal']/namePart[@type='given']",
    "/mods/name[2][@type='personal']/namePart[@type='family']",
    "/mods/originInfo/dateCreated" );

  $data = array(
    "1794 1",
    "A View of Religions in Two Parts",
    "",
    "A view of religions, in two parts. : Part I. Containing an alphabetical compendium of the various religious denominations, which have appeared in the world, from the beginning of the Christian era to the present day. Part II. Containing a brief account of the different schemes of religion now embraced among mankind. The whole collected from the best authors, ancient and modern",
    "Hannah",
    "Adams",
    "1791");

  if ($pid = icg_csv_import_batch_item($xpaths,$data,'icg-collection:1')) {
    drupal_set_message("Yes!  New object '$pid' has been generated!", 'status');
    return TRUE;
  } else {
    drupal_set_message("Drats!  The process failed to create a new object.", 'error');
    return FALSE;
  }

}

