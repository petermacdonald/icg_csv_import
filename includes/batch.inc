<?php

/**
 * David's readCSV() function.
 *
 * @param $csv_file
 * @return bool
 */
function icg_csv_import_readCSV($csv_file, $parent, $ns) {
  //$csv_file = "valid_example1";
  ini_set("auto_detect_line_endings", true);
  $csv_data = array();
  $rowcount = 0;

  //open csv file
  if (($file_handle = fopen($csv_file, "r")) !== FALSE) {

    //read first line to get header names and number of headers
    $csv_headers = fgetcsv($file_handle);
    $csv_headers_number = count($csv_headers);
    $entry = array();

    //check to see if number of row data matches number of headers
    while (($row = fgetcsv($file_handle)) !== FALSE) {
      $number_of_row_data = count($row);

      //if numbers match, enter into arrays
      if ($number_of_row_data == $csv_headers_number) {
        $entry = $row;
//      $csv_data[] = $entry;
        $pid = icg_csv_import_batch_item($csv_headers, $entry, $parent, $ns, $inactive=FALSE);
      }
      //if numbers don't match
      else {
        echo("CSV Reader: Invalid number of columns at line " . ($rowcount + 2) . " (, row " . ($rowcount + 1) . ") in ".$csv_file.". Expected=$csv_headers_number; Got=$number_of_row_data");
        return FALSE;
      }
      $rowcount++;
    }
    /*****************************************
    //display data to check it (can be deleted)
    echo "Total rows found: $rowcount\n";
    for ($row = 0; $row < sizeof($csv_data); $row++) {
    echo "<p><b>Row number $row</b></p>";
    echo "<br>";
    print_r ($csv_data[$row]);
    for ($col = 0; $col < sizeof($csv_data[$row]); $col++) {
    echo "<li>".$csv_data[$row][$col]."</li>";
    }
    echo "<br>";
    }
    //end data display
     ******************************************/

    //close file connection
    fclose($file_handle);
  }
  else {
    echo("CSV Reader: Could not read CSV file: ".$csv_file.".csv");
    return FALSE;
  }
  return TRUE;
}

/**
 * Handle form submission. Read the CSV into a set of batch operations
 * and fire them off.
 *
 * Lifted from https://github.com/GiantRobot/icg_csv_import/blob/7.x-1.x/icg_csv_import.module
 *
 * function icg_csv_import_form_submit(&$form, &$form_state) {
 */
function icg_csv_import_build_batch($form_state) {
  $module_name = basename(__FILE__, '.module');
  static $temporary = '/tmp';
  static $update;

  $batch = array(
    'title' => t('Importing CSV ...'),
    'operations' => array(),
    'init_message' => t('Commencing'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('An error occurred during processing'),
    'finished' => 'icg_csv_import_import_finished',
  ) ;

  $csv_filename = $form_state['all_values']['csv_file']->uri;
  
  if ($handle = fopen($csv_filename, 'r')) {
    if ($form_state['all_values']['skip_field_mapping']) {
      $headers = fgetcsv($handle);   // first line is $headers
    } else {
      $headers = $form_state['all_values']['crosswalk'];  // get $headers from the crosswalk
    }

    $format = 'Y-m-d:H:i:s';
    $marker = date($format);

    // Open a temporary .csv file and write the $marker cell followed by $headers.
    $history = $temporary.'/CSV_import_history_'.$marker.'.csv';
    if (!$temp = fopen($history,'w')) {
      watchdog($module_name, "Could not create CSV history file '%history'.", array('%history' => $history), WATCHDOG_ERROR);
      return FALSE;
    }

    if (!$fp = fopen($history, 'w')) {
      watchdog($module_name, "Could not open CSV history file '%history'.", array('%history' => $history), WATCHDOG_ERROR);
      return FALSE;
    }
    // debug("Successfully opened file '$history' to capture CSV import status.");

    $record = $headers;

    // Determine if a date/time marker is already present in the first $headers cell.  If yes,
    // this process will update the first column, otherwise it will append a new column of data
    // the the history.
    if ($previous = strtotime($record[0])) {
      $update = TRUE;
      $record[0] = $marker;
    } else {
      $update = FALSE;
      array_unshift($record, $marker);
    }

    // Write the $headers to the history file and close it.
    fputcsv($fp, $record);
    fclose($fp);

    // Get the parent_pid parameter and save it.  @TODO...remove, no longer needed
    // $parent_pid = $form_state['build_info']['args'][0]->id;
    // $form_state['all_values']['parent_pid'] = $parent_pid;

    // First batch operation...save key parameters
    $form_state['all_values']['history'] = $history;
    $form_state['all_values']['update'] = $update;
    $batch['operations'][] = array('icg_csv_import_remember', array($csv_filename, $form_state['all_values'], $headers));

    // Read and process the CSV file one line at a time
    while ($line = fgetcsv($handle)) {
      // use base64_encode to ensure we don't overload the batch processor by stuffing complex objects into it
      $batch['operations'][] = array('icg_csv_import_import_line', array(array_map('base64_encode', $line)));
    }
    fclose($handle);
  }

  // Return path
  $path = '/islandora/object/'.$form_state['all_values']['parent_pid'];
  
  // Set for batch processing...and Go!
  batch_set($batch);
  batch_process($path);
}

/**
 * Handle batch completion.
 *
 * Lifted from https://github.com/GiantRobot/icg_csv_import/blob/7.x-1.x/icg_csv_import.module
 */
function icg_csv_import_import_finished($success, $results, $operations) {
  if ( !empty($results['failed_rows']) ) {
    $dir = 'public://icg_csv_import' ;
    if (file_prepare_directory( $dir, FILE_CREATE_DIRECTORY ) ) {
      $csv_filename = 'failed_rows-'. basename($results['uploaded_filename']); // we validated extension on upload
      $csv_filepath = $dir .'/'. $csv_filename;
      $targs = array(
        '!csv_url' => l(check_plain($csv_filename), file_create_url($csv_filepath)),
        '%csv_filename' => $csv_filename,
        '%csv_filepath' => $csv_filepath,
      ) ;
      if ( $handle = fopen($csv_filepath, 'w+') ) {
        foreach( $results['failed_rows'] as $failed_row ) {
          fputcsv($handle, $failed_row);
        }
        fclose($handle);
        drupal_set_message(t('Some rows failed to import. You may download a CSV of these rows: !csv_url', $targs), 'error');
      }
      else {
        drupal_set_message(t('Some rows failed to import, but unable to write error CSV to %csv_filepath', $targs), 'error');
      }
    }
    else {
      drupal_set_message(t('Some rows failed to import, but unable to create directory for error CSV at %csv_directory', array('%csv_directory' => $dir)), 'error');
    }
  }
  return t('The CSV import has completed.');
}

/**
 * Remember the uploaded CSV filename, control parameters, and header.
 *
 * Lifted from https://github.com/GiantRobot/icg_csv_import/blob/7.x-1.x/icg_csv_import.module
 *
 */
function icg_csv_import_remember($filename, $parameters, $headers, &$context) {
  $context['results']['uploaded_filename'] = $filename;
  $context['results']['parameters'] = $parameters;
  $context['results']['headers'] = $headers;
}

/**
 * Process a single line.
 *
 * Lifted from https://github.com/GiantRobot/icg_csv_import/blob/7.x-1.x/icg_csv_import.module
 */
function icg_csv_import_import_line($line, &$context) {
  $context['results']['rows_imported']++;
  $line = $cleaned_line = array_map('base64_decode', $line);
  
  // dpm($context);

  // Show the import row count
  $context['message'] = t('Importing row !c', array( '!c' => $context['results']['rows_imported'] ));
  
  // Pull parameters from $context...
  $headers = $context['results']['headers'];

  // Process this line of CSV data.  If the import fails, save the $line in the failed_rows array.
  if (!$pid = icg_csv_import_batch_item($headers, $line, $context['results']['parameters'])) {
    $context['results']['failed_rows'][] = $line;
  }
}

/**
 * icg_csv_import_batch_item - Generate one new object in Islandora from CSV data.
 *
 * Generates one new object in the Islandora repository based on the XPaths ($headers)
 * of the destination metadata fields and corresponding $row data.
 *
 * @param array() $headers - XPaths of destination fields
 * @param array() $row - One CSV row of corresponding data to deliver to destination fields
 * @param array() $controls - Process controls passed from the user interface. Elements mostly
 *   from $form_state['all_values'] include:
 * 
 *   parent_pid - The PID of the parent collection
 *   namespace - The namespace (no colon) to ingest new objects into
 *   inactive - Indicator if the ingested objects are to be Active or Inactive
 *   history - Name of the temporary file holding processed import history
 *   update - Indicates if this process is updating a previous import (has history) or if objects are all new
 *   transform  @TODO - The path to the defined XSLT transform to be applied to new MODS records
 *   labelXPath  @TODO - The XPath to the data to be used as new object labels
 *
 * @return string|bool - The PID of the ingested object, or FALSE in case of an error
 */
function icg_csv_import_batch_item($headers, $row, $controls) {
  $module_name = basename(__FILE__, '.module');
  static $temporary = '/tmp';
  static $MODS = '/templates/MODS.xml';

  $parent_pid = $controls['parent_pid'];
  $namespace = $controls['namespace'];
  $inactive = $controls['ingest_as_inactive'];
  $history = $controls['history'];
  $update = $controls['update'];
  $transform = $controls['transform'];   
  $labelXPath = $controls['label_field'];
  $credentials['username'] = $controls['credentials_username'];
  $credentials['password'] = $controls['credentials_password'];

  $format = 'Y-m-d:H:i:s';
  $marker = date($format);
  $path = drupal_get_path('module','icg_csv_import');

  $record = $row;

  // Check for a hashtag (comment) marker at the start of $row.  If found then echo
  // $row into the history file but do nothing else.
  if (_startsWith($record[0],'#')) {
    $fp = fopen($history,'a');    // open the history CSV file to append data
    if (!$update) array_unshift($record, '#');
    fputcsv($fp, $record);            // append $record as-is to the history
    fclose($fp);
    return TRUE;
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
  $obj = $cmodel = $label = NULL;

  // Primary loop...where the rubber meets the road.
  //
  // Loop $i through $headers and $row adding data elements to $doc along the way.
  // Skip all comment (# as first character) rows and empty $headers or $row cells.
  // Look for the optional "special" $headers 'LABEL, 'OBJ' and 'CModel' which mark the
  // columns of $row that contain the object label, OBJ content filename, and
  // content model, respectively.

  for ($i=0, $parent=NULL; $i<$length; $i++) {
    if (empty($headers[$i]) || _startsWith($headers[$i], '#')) { continue; }   // $headers[$i] is blank or a comment...skip this column
    if (empty($row[$i])) { continue; }   // $row[$i] is empty...skip it
    if ($headers[$i] === "LABEL") {
      $label = $row[$i];
    } else if ($headers[$i] === "OBJ") {
      $obj = $row[$i];
    } else if ($headers[$i] === "CMODEL") {
      $cmodel = $row[$i];
    } else {
      if ($headers[$i] === $labelXPath) { $label = $row[$i]; }
      $parent = _generateNodeFromXPath($doc, $parent, $headers[$i], $row[$i]);
    }
  }

  // Save $doc as a temporary file for debugging purposes.
  $temp = drupal_tempnam($temporary, 'XML_');
  $mods = $doc->saveXML();
  if (!file_put_contents($temp, $mods)) {
    watchdog($module_name, "Could not save XML as a temporary text file.", array( ) , WATCHDOG_WARNING);
  } else {
    debug("Complete XML saved as '$temp'.");
  }

  // Create a new object with $mods as it's MODS datastream and return the object's PID.
  if ($pid = _icg_make_new_object($parent_pid, $namespace, $cmodel, $label, $mods, $transform, $obj, $credentials, $inactive)) {
    if ($update) {
      $record[0] = $pid;
    } else {
      array_unshift($record, $pid);
    }
  } else {
    if ($update) {
      $record[0] = 'Import Failed';
    }
    else {
      array_unshift($record, 'Import Failed');
    }
  }

  // Append this $row result to the history
  $fp = fopen($history, 'a');    // open the history CSV file to append data
  fputcsv($fp, $record);         // append the $pid and data ($row)
  fclose($fp);

  return $pid;
}

// ------------------------ Private functions ----------------------------

/**
 * Call any/all registered hook_fetch_OBJ functions.
 *
 * @param string $path - Path of the content (file) to fetch.
 * @param array $credentials
 *   'username' => The username to use for successful file transfer.
 *   'password' => The corresponding password for file transfer.
 */
function _fetch_OBJ($path, $credentials) {
  // Call all modules implementing 'hook_fetch_OBJ':
  $return = module_invoke_all('fetch_OBJ', $path, $credentials);
  return $return[0];
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
function _generateNodeFromXPath($doc, $parent, $xpath, $data=NULL, $previous='' ) {
  static $ns = 'mods';

  // If no parent is provided, assume that we begin from the document root
  if (!$parent) { $parent = $doc->documentElement; };

  // Trim the $xpath
  $trimmed = trim($xpath, '/');

  // Grab the next element name in the $xpath
  $parts = _multiexplode(array('/','[',']'), $trimmed);

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
  if (!empty($parts[0]) && _startsWith($parts[0], '@')) {   // element has an attribute
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
      $parent = _make_node($doc, $parent, $element, $attribute, $text);
    }
  } else {   // We have an $index!  We need a new $node attached to $parent
    $parent = _make_node($doc, $parent, $element, $attribute, $text);
  }

  // If there are any $parts left, build the $remainder xpath, pass $current as the last xpath processed, and recurse.
  if (!empty($parts[0])) {
    $remainder = implode('/',$parts);
    _generateNodeFromXPath($doc, $parent, $remainder, $data, $current);
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
 * @param string $transform - Path to the MODS-to-DC transform xslt.
 * @param string $obj - The name of the file holding the OBJ datastream content.
 * @param array $credentials
 *   'username' => The username to use for successful file transfer.
 *   'password' => The corresponding password for file transfer.
 * @param bool $inactive - TRUE if the object is to be made inactive.
 * 
 * @return mixed - The PID of the new object, or FALSE if the process failed.
 */
function _icg_make_new_object($parent_pid, $ns, $cmodel, $label, $mods, $transform, $obj=NULL, $credentials=NULL, $inactive=FALSE) {
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
  // $object from it.  @TODO...MIME type assumes image/jpeg!
  if ($obj) {
    if ($temp = _fetch_OBJ($obj, $credentials)) {
      $dsid = 'OBJ';
      $datastream = $object->constructDatastream($dsid);
      $datastream->label = 'Object Content';
      $datastream->mimeType = 'image/jpeg';
      $datastream->setContentFromFile($temp, TRUE);
      $object->ingestDatastream($datastream);
    }
  }

  // Ingest the new object into Fedora.
  $pid = $object->id;
  try {
    $repo->ingestObject($object);
  } catch (Exception $e) {
    watchdog($module_name, "Caught Fedora exception: %e.", array('%e' => $e->getMessage()), WATCHDOG_ERROR);
    return FALSE;
  }

  // Now, get the MODS-to-DC transform.  
  $xsl = new DOMDocument( );
  if (!$xsl->load($transform)) { die("Failed to load Transform file '$transform!'"); }
  $xslt = new XSLTProcessor( );
  $xslt->importStyleSheet($xsl);

  // Apply the MODS-to-DC transform.
  $document = new DOMDocument( );
  $document->loadXml($object['MODS']->content);
  if ($doc = $xslt->transformToDoc($document)) {
    $after = $doc->saveXML();
    $object['DC']->setContentFromString($after);
  }

  // OK, my hook_CMODEL_PID_islandora_object_ingested function is NOT firing!  So
  // take the bull by the horns here.  
  _make_derivatives($object);

  // Now, if the object is to be "inactive", set it so.
  if ($inactive) {
    $api_m = $object->repository->api->m;
    $owner = $object->owner;
    $timestamp = $api_m->modifyObject($pid, "I", $owner, "Object set inactive via CSV import.");
  }

  return $pid;
}

/**
 * Make derivatives.
 *
 * @param AbstractObject $object - The ingested object in need of derivatives.
 */
function _make_derivatives($object) {
  $module_name = basename(__FILE__, '.module');
  $cmodels = $object->models;
  foreach ($cmodels as $cmodel) {
    switch ($cmodel) {
      case 'islandora:sp_basic_image':
        module_load_include('inc', 'islandora_basic_image', 'includes/derivatives');
        islandora_basic_image_create_thumbnail($object);
        islandora_basic_image_create_medium_size($object);
        return TRUE;
        break;
      default:
        watchdog($module_name, "Derivatives cannot be created for object '%pid' with CModel type '%cmodel'.", array(
          '%pid' => $object->id,
          '%cmodel' => $cmodel
        ), WATCHDOG_WARNING);
        return FALSE;
        break;
    }
  }
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
function _make_node($doc, $parent, $element, $attribute, $value=NULL) {
  // if (empty($value) && empty($attribute)) { return $parent; }  // no $value or $attribute, return the $parent
  $node = $doc->createElement($element);   // create a new $node with name $element
  $text = $doc->createTextNode(htmlentities($value));   // save $value as new $text node
  $node->appendChild($text);   // attach the $value to the new $node
  if ($attribute) {
    $node->appendChild($attribute);     // attach $attribute to the new $node
  }
  $node = $parent->appendChild($node);  // attach the new $node to $parent and return it
  return $node;
}

/**
 * Explode a string using multiple delimiters.  This code modified from
 * http://php.net/manual/en/function.explode.php
 *
 * @param $delimiters
 * @param $string
 * @return array
 */
function _multiexplode($delimiters, $string) {
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
 * Determine if a string starts with a particular character.  This code lifted
 * from http://stackoverflow.com/questions/834303/startswith-and-endswith-functions-in-php
 *
 * @param $haystack
 * @param $needle
 * @return bool
 */
function _startsWith($haystack, $needle) {
  // search backwards starting from haystack length characters from the end
  return $needle === "" || strrpos($haystack, $needle, -strlen($haystack)) !== false;
}

