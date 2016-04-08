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
 * @param bool $inactive - Indicates if the new object is to be active (default) or inactive
 *
 * @return string|bool - The PID of the ingested object, or FALSE in case of an error
 */
function icg_csv_import_batch_item($headers, $row, $parent_pid, $namespace='test', $inactive=FALSE) {

  static $history;
  static $MODS = '/templates/MODS.xml';

  $marker = date('Y-m-d:H:i:s');
  $path = drupal_get_path('module','icg_csv_import');

  $module_name = basename(__FILE__, '.module');

  // On the first call to this function, open a temporary .csv file and write the
  // $marker cell followed by $headers.
  if (empty($history)) {
    $history = 'public://history_'.$marker.'.csv';
    if (!$temp = fopen($history,'w')) {
      watchdog($module_name, "Could not create CSV history file '%history'.", array('%history' => $history), WATCHDOG_ERROR);
      return FALSE;
    }
    
    if (!$fp = fopen($history, 'w')) {
      watchdog($module_name, "Could not open CSV history file '%history'.", array('%history' => $history), WATCHDOG_ERROR);
      return FALSE;
    } 
    
    debug("Successfully opened file '$history' to capture CSV import status.");
    $record = $headers;
    array_unshift($record, $marker);
    fputcsv($fp, $record);
    fclose($fp);
  }

  // Fetch the MODS template
  if (!$template = file_get_contents($path.$MODS)) {
    watchdog($module_name, "Could not open '%mods'.", array('%mods' => $MODS), WATCHDOG_ERROR);
    return FALSE;
  }

  // Use $MODS to create a new DOMDocument to hold the data.
  $doc = new DOMDocument();
  if (!$doc->loadXML($template)) {
    watchdog($module_name, "Could not load '%mods' content as XML.", array('%mods' => $MODS), WATCHDOG_ERROR);
    return FALSE;
  }

  $length = min(count($headers), count($row));
  $obj = $cmodel = NULL;

  // Loop $i through $headers and $row adding data elements to $doc along the way.
  // Look for the optional "special" $headers 'OBJ' and 'CModel' which mark the
  // columns of $row that contain the OBJ content filename and content model, respectively.
  for ($i=0, $parent=NULL; $i<$length; $i++) {
    if ($headers[$i] === "OBJ") {
      $obj = $headers[$i];
    } else if ($headers[$i] === "CModel") {
      $cmodel = $headers[$i];
    } else {
      $parent = GenerateNodeFromXPath($doc, $parent, $headers[$i], $row[$i]);
    }
  }

  // Save $doc as a temporary file for debugging purposes.
  $temp = drupal_tempnam('temporary://', 'XML_');
  $mods = $doc->saveXML();
  if (!file_put_contents($temp, $mods)) {
    watchdog($module_name, "Could not save XML as a temporary text file.", array( ) , WATCHDOG_WARNING);
  } else {
    debug("Complete XML saved as '$temp'.");
  }

  // Create a new object with $mods as it's MODS datastream and return the object's PID.
  // @TODO...need a content_model and label here!
  if ($pid = icg_make_new_object($parent_pid, $namespace, $cmodel, 'Test', $mods, $obj)) {
    $fp = fopen($history,'a');    // open the history CSV file to append data
    $record = $row;
    array_unshift($record, $pid);
    fputcsv($fp,$record);         // append the $pid and data ($row)
    fclose($fp);
  }

  return $pid;
}

/**
 * Generate a new XML node from an XPath specification.
 * @TODO...MODS is assumed to be the target schema.
 *
 * @param $doc
 * @param $parent
 * @param $xpath
 * @param null $data
 * @param string $previous
 */
function generateNodeFromXPath( $doc, $parent, $xpath, $data=NULL, $previous='' ) {
  static $ns = 'mods';

  // If no parent is provided, assume that we begin from the document root
  if (!$parent) { $parent = $doc->documentElement; };

  // Trim the $xpath
  $trimmed = trim($xpath, '/');

  // Grab the next element name in the $xpath
  $parts = multiexplode(array('/','[',']'), $trimmed);

  // Pop the $element off of the $parts array
  $element = array_shift($parts);
  // dpr("element:$element\n");

  $index = $attr = $attribute = FALSE;

  // Look ahead for a node $index in $parts; don't add these to $previous
  if (!empty($parts[0]) && is_numeric($parts[0])) {    // element has an index
    $index = array_shift($parts);
    // dpr("index:$index\n");
  }

  // Look ahead for an $attribute in $parts
  if (!empty($parts[0]) && startsWith($parts[0], '@')) {   // element has an attribute
    list($attr, $val) = explode('=', $parts[0], 2);
    $value = trim($val,"'");
    // dpr("attribute:$attr=$value\n");
    array_shift($parts);
  }

  // If there is an $attribute, build it's XML.
  if ($attr) {
    $attribute = $doc->createAttribute(trim($attr,"@"));
    $attribute->value = $value;
  }

  // Add the $ns namespace to this element and then append that to $previous to make a new $current xpath
  if ($element) { $current = $previous.'/'.$ns.':'.$element; }
  if ($attr) { $current .= "[$attr='$value']"; }
  // dpr("current:$current\n");

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
 * Make a new Fedora object with specified characteristics and belonging to an existing
 * parent object (presumably a 'collection').
 *
 * @TODO... MODS is assumed here!
 *
 * @param $parent_pid - The PID of the new object's parent.  Presumably this is a collection object.
 * @param $ns - The namespace (no trailing colon) of the new object.
 * @param string $cmodel - The content model to be associated with the new object.
 * @param string $label - The label (title?) to be applied to the Fedora object.
 * @param string $mods - The MODS record content as a string.
 * @param string $obj - The name of the file holding the OBJ datastream content.
 * @return mixed - The PID of the new object, or FALSE if the process failed.
 */
function icg_make_new_object($parent_pid, $ns, $cmodel, $label, $mods, $obj=NULL) {
  $module_name = basename(__FILE__, '.module');

  $content_model = (!$cmodel ? 'islandora:compoundCModel' : $cmodel);
  $collection_object = islandora_object_load($parent_pid);
  if ($collection_object) {
    $repo = $collection_object->repository;
  }
  else {
    watchdog($module_name, "Could not open parent object '%parent'.", array('%parent' => $parent_pid), WATCHDOG_ERROR);
    return FALSE;
  }
  $object = $repo->constructObject($ns);    // construct a new object in the specified namespace
  $object->models = $content_model;         // associate it with $content_model
  $object->label = $label;                  // use the label provided

  // Make a new MODS datastream for $object from $doc.
  $dsid = 'MODS';
  $datastream = $object->constructDatastream($dsid);
  $datastream->label = 'MODS Record';
  $datastream->mimeType = 'text/xml';
  $datastream->setContentFromString($mods);
  $object->ingestDatastream($datastream);

  // Set the object's isMemberOfCollection relation so it becomes part of
  // the $parent_pid collection.
  $object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $parent_pid);

  // If $obj is specified, fetch the file contents and make a new OBJ datastream for
  // $object from it.  @TODO...Assumes image/jpeg!
  if ($obj) {
    $dsid = 'OBJ';
    $datastream = $object->constructDatastream($dsid);
    $datastream->label = 'Object Content';
    $datastream->mimeType = 'image/jpeg';
    $datastream->setContentFromFile($obj, TRUE);
    $object->ingestDatastream($datastream);
  }

  // Ingest the new object into Fedora.
  $pid = $object->id;
  try {
    $repo->ingestObject($object);
  } catch (Exception $e) {
    watchdog($module_name, "Caught Fedora exception: %e.", array('%e' => $e->getMessage()), WATCHDOG_ERROR);
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
 * @return DOMElement - Returns the new XML element.
 */
function make_node($doc, $parent, $element, $attribute, $value=NULL) {
  $node = $doc->createElement($element, $value);   // create a new $node with name $element
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

/**
 * Fetch the named file from storage using FTP.
 * 
 * @TODO...This needs to be a hook so that others can replace it!
 * 
 * @param string $filename - File name to fetch via FTP.
 */
function fetch_OBJ($filename) {
  static $ftp_server = '132.161.10.13';
  static $ftp_directory = '\MEDIADB';
  static $ftp_username = 'user';
  static $ftp_userpass = 'password';

  $module_name = basename(__FILE__, '.module');

  // Set up a basic connection.
  $conn_id = ftp_ssl_connect($ftp_server, 21, 180);
  if (!$conn_id) {
    watchdog($module_name, "FTP connection failed!", array( ), WATCHDOG_ERROR);
    return FALSE;
  }

  // Login with the specified FTP username and password.
  $login_result = ftp_login($conn_id, $ftp_username, $ftp_userpass);
  if (!$login_result) {
    watchdog($module_name, "FTP login failed!", array( ), WATCHDOG_ERROR);
    return FALSE;
  }

  // Attempt to change the remote directory as specified.
  $change_dir = ftp_chdir($conn_id, $ftp_directory);
  if (!$change_dir) {
    watchdog($module_name, "FTP chdir to '%directory' failed!", array('%directory' => $ftp_directory), WATCHDOG_ERROR);
    return FALSE;
  }

  // Fetch a file via FTP.
  $temp_file = drupal_tempnam('temporary://', 'import_content_');   // open a temp file
  if (!ftp_get($conn_id, $temp_file, $filename, FTP_BINARY)) {
    watchdog($module_name, "Could not download file '%filename' via FTP.", array('%filename' => $filename), WATCHDOG_ERROR);
    return FALSE;
  }
  
  return $temp_file;
}

