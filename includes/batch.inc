<?php

/**
 * icg_csv_import_batch_item
 * Generates one new object in the Islandora repository based on the Xpaths ($headers)
 * of the destination data fields and corresponding $row data.
 *
 * @param array() $headers - Xpaths of destination fields
 * @param array() $row - One CSV row of corresponding data to deliver to destination fields
 * @param ??? $xml_form_id - @TODO...TBD. Presumably the form ID that the datastream's XML schema can be derived from
 * @param IslandoraFedoraObject $parent_object  - The collection object that the new object will belong to
 * @param string $namespace - The namespace, no trailing colon, that the new object will be defined in
 * @param bool $inactive - Indicates if the new object is to be active (default) of inactive
 * @return string|bool - The PID of the ingested object, or FALSE in case of an error
 */
function icg_csv_import_batch_item($headers, $row, $xml_form_id, $parent_object, $namespace, $inactive=FALSE) {

  /* @TODO...Improve this!
   * Ideally we would fetch a template XML structure and target DSID (?) from $xml_form_id but I could
   * find no easy way to do that.  So I'm taking the easy road, for now, and will assume the data is MODS, and the
   * use ../templates/MODS.xml to represent the XML structure.
   */

  $template = "../templates/MODS.xml";
  $template_xml = fetch_template($template);
  $dx = new DOMXPath($template_xml);

  // Loop $i through $headers and $row.
  $length = min(count($headers), count($row));

  for ($i=0; $i<$length; $i++) {

    // Search $template_xml for the Xpath specifed in $headers[$i].
    $xpath = $headers[$i];
    $nodes = $dx->query($xpath);    // $nodes is a DOMNodeList

    if ($nodes->length < 1) {
      drupal_set_message("Xpath '$xpath' not found in template '$template'.", "error");
      return FALSE;
    }

    $node = $nodes->item(0);  // $node is a single DOMNode   @TODO Set breakpoint here and see what is returned!

    // Assuming $node is a DOMNode...
    $clone = $node->cloneNode(FALSE);  // $clone is a new DOMNode
    $clone->nodeValue = $row[$i];

    // @TODO Attach the $clone to the DOMDocument

  } // for ($i=0...

  // @TODO Save the DOMDocument as the object's MODS datastream

  return FALSE;
}


/**
 * Return the validated/sanitized contents of the template file.
 *
 * @return DOMDocument - The template contents as a DOMDocument.
 */
function fetch_template($template) {
  $contents = file_get_contents($template);
  // @TODO...validate/sanitize the template?
  if (strlen($contents) < 1) {
    die("Failed to open and read template file '$template'!");
  }

  $xml = new DOMDocument;
  // Open template as searchable XML
  if (!$xml->loadXML($contents)) {
    die ("Failed to create valid XML from template file '$template'!"); 
  }
  return $xml;
}

