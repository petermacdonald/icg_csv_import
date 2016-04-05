<?php

/**
 * icg_csv_import_batch_item - Generate one new object in Islandora from CSV data.
 *
 * Generates one new object in the Islandora repository based on the XPaths ($headers)
 * of the destination metadata fields and corresponding $row data.
 *
 * @param array() $headers - XPaths of destination fields
 * @param array() $row - One CSV row of corresponding data to deliver to destination fields
 * @param string $parent_pid  - The collection object, presumably, that the new object will belong to
 * @param string $namespace - The namespace, no trailing colon, that the new object will be defined in
 * @param bool $inactive - Indicates if the new object is to be active (default) of inactive
 * @return string|bool - The PID of the ingested object, or FALSE in case of an error
 */
function icg_csv_import_batch_item($headers, $row, $parent_pid, $namespace=NULL, $inactive=FALSE) {
  static $MODS = '/templates/MODS.xml';
  
  $path = drupal_get_path('module','icg_csv_import');

  // Fetch the MODS template
  if (!$template = file_get_contents($path.$MODS)) {
    drupal_set_message("Could not open '$MODS'.", 'error');
    return FALSE;
  }

  // Use $MODS to create a new DOMDocument to hold the data.
  $doc = new DOMDocument();
  if (!$doc->loadXML($template)) {
    drupal_set_message("Could not load '$MODS' content as XML.", 'error');
    return FALSE;
  }

  // Loop $i through $headers and $row adding data elements to $doc along the way.
  $length = min(count($headers), count($row));
  for ($i=0, $parent=NULL; $i<$length; $i++) {
    $parent = GenerateNodeFromXPath($doc,$parent,$headers[$i],$row[$i]);
  }

  // Save $doc as a temporary file for debugging purposes.
  $temp = drupal_tempnam('temporary://', 'XML_');
  $mods = $doc->saveXML();
  if (!file_put_contents($temp, $mods)) {
    drupal_set_message("Could not save XML as a temporary text file.", 'error');
    return FALSE;
  } else {
    drupal_set_message("Complete XML saved as '$temp'.", 'status');
  }

  // Create a new object with $mods as it's MODS datastream and return the object's PID.
  // @TODO...need a content_model and label here!
  return icg_make_new_object($parent_pid, 'islandora:compoundCModel', 'Test', $mods);
}

/**
 * Generate a new XML node from an XPath specification.
 *
 * @param $doc
 * @param $parent
 * @param $xpath
 * @param null $data
 * @param string $previous
 */
function generateNodeFromXPath( $doc, $parent, $xpath, $data=NULL, $previous='' ) {
  static $ns = 'mods';   // @TODO...namespace $ns is assumed here

  // If no parent is provided, assume that we begin from the document root
  if (!$parent) { $parent = $doc->documentElement; };

  // Trim the $xpath
  $trimmed = trim($xpath, '/');

  // Grab the next element name in the $xpath
  $parts = multiexplode(array('/','[',']'), $trimmed);

  // Pop the $element off of the $parts array
  $element = array_shift($parts);
  dpr("element:$element\n");

  $index = $attr = $attribute = FALSE;

  // Look ahead for a node $index in $parts; don't add these to $previous
  if (!empty($parts[0]) && is_numeric($parts[0])) {    // element has an index
    $index = array_shift($parts);
    dpr("index:$index\n");
  }

  // Look ahead for an $attribute in $parts
  if (!empty($parts[0]) && startsWith($parts[0], '@')) {   // element has an attribute
    list($attr, $val) = explode('=', $parts[0], 2);
    $value = trim($val,"'");
    dpr("attribute:$attr=$value\n");
    array_shift($parts);
  }

  // If there is an $attribute, build it's XML.
  if ($attr) {
    $attribute = $doc->createAttribute(trim($attr,"@"));
    $attribute->value = $value;
  }

  // Add the mods: namespace to this element and then append that to $previous to make a new $current xpath
  if ($element) { $current = $previous.'/'.$ns.':'.$element; }
  if ($attr) { $current .= "[$attr='$value']"; }
  dpr("current:$current\n");

  // If this is not the end of the $xpath, don't set $data!
  if (!empty($parts[0])) {
    $text = NULL;
  } else {
    $text = $data;
  }

  // If there is no $index, then look for $current.
  if (!$index) {
    $dx = new DOMXPath($doc);
    $nodes = $dx->query($current);
    if ($nodes->length > 0) {     // $current was found, set $parent to the $current node and move on.
      $parent = $nodes->item(0);
    } else {    // $current not found, append a new child node to $parent and make it the new $parent.
      $parent = make_node($doc, $parent, $element, $attribute, $text);
    }
  } else {   // We have an $index!  We need a new $node attached to $parent
    $parent = make_node($doc, $parent, $element, $attribute, $text);
  }

  // If there are any $parts left, build the $remainder xpath, pass $current as the last xpath processed, and recurse.
  if (!empty($parts[0])) {
    $remainder = implode('/',$parts);
    generateNodeFromXPath($doc, $parent, $remainder, $data, $current);
  }
}


/**
 * Make a new Fedora object in the namespace of an existing parent object.
 *
 * @param $parent_pid - The PID of the new object's parent.  Presumably this is a collection object.
 * @param string $content_model - The content model to be associated with the new object.
 * @param string $label - The label (title?) to be applied to the Fedora object.
 * @param string $mods - The MODS record content as a string.
 * @return mixed - The PID of the new object, or FALSE if the process failed.
 */
function icg_make_new_object($parent_pid, $content_model, $label, $mods) {
  $collection_object = islandora_object_load($parent_pid);
  if ($collection_object) {
    $repo = $collection_object->repository;
  }
  else {
    drupal_set_message("Could not open parent object '$parent_pid'.", 'error');
    return FALSE;
  }
  list($ns, $pidn) = explode(':', $parent_pid);
  $object = $repo->constructObject($ns);         // construct a new object in the parent's namespace
  $object->models = $content_model;              // associate it with $content_model
  $object->label = $label;                       // use the label provided

  // Make a new MODS datastream for $new_object from $doc.
  $dsid = 'MODS';
  $datastream = $object->constructDatastream($dsid);
  $datastream->label = 'MODS Record';
  $datastream->mimeType = 'text/xml';
  $datastream->setContentFromString($mods);
  $object->ingestDatastream($datastream);

  // Ingest the new object into Fedora.
  $pid = $object->id;
  try {
    $repo->ingestObject($object);
  } catch (Exception $e) {
    drupal_set_message('Caught Fedora exception: ' . $e->getMessage(), 'error');
    return FALSE;
  }

  return $pid;
}


/**
 * Explode a string using multiple delimiters.  This code modified from
 * http://php.net/manual/en/function.explode.php
 *
 * @param $delimiters
 * @param $string
 * @return array
 */
function multiexplode($delimiters, $string) {
  $ready = str_replace($delimiters, $delimiters[0], $string);
  $parts = explode($delimiters[0], $ready);
  $stack = array( );
  foreach($parts as $part) {
    $p = trim($part," /[]");
    if (strlen($p)>0) {
      $stack[] = $p;
    }
  }
  return $stack;
}

/**
 * Make a new XML node to represent $element with an optional $value.
 *
 * @param $doc
 * @param $parent
 * @param $element
 * @param $attribute
 * @param null $value
 * @return mixed -
 */
function make_node($doc, $parent, $element, $attribute, $value=NULL) {
  $node = $doc->createElement($element, $value);   // create a new $element $node
  if ($attribute) {
    $node->appendChild($attribute);     // attach $attribute to the new $node
  }
  $node = $parent->appendChild($node);  // attach the new $node to $parent and return it
  return $node;
}

/**
 * Determine if a string starts with a particular character.  This code lifted
 * from http://stackoverflow.com/questions/834303/startswith-and-endswith-functions-in-php
 *
 * @param $haystack
 * @param $needle
 * @return bool
 */
function startsWith($haystack, $needle) {
  // search backwards starting from haystack length characters from the end
  return $needle === "" || strrpos($haystack, $needle, -strlen($haystack)) !== false;
}



function test() {
  if (!$xp = file_get_contents('/var/www/drupal/sites/all/modules/icg_csv_import/templates/MODS.xml')) {
    drupal_set_message("Could not open 'MODS.xml'.", 'error');
    return;
  }
  $xml = new DOMDocument( );
  if (!$xml->loadXML($xp)) {
    drupal_set_message("Could not load 'MODS.xml' as a DOMDocument.", 'error');
    return;
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
  } else {
    drupal_set_message("Drats!  The process failed to create a new object.", 'error');
  }

}