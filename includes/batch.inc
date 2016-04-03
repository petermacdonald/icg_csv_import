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
  static $XPToXML = '/includes/XPathsToXML.xsl';
  static $MODS = '/templates/MODS.xml';
  
  $path = drupal_get_path('module','icg_csv_import');
 
  // Fetch the XPathsToXML.xsl XSL template
  if (!$xp = file_get_contents($path.$XPToXML)) {
    drupal_set_message("Could not open '$XPToXML'.", 'error');
    return FALSE;
  } 

  // Load the XSL template as XML
  $xsl = new DOMDocument( );
  if (!$xsl->loadXML($xp)) {
    drupal_set_message("Could not load '$xp' as a DOMDocument.", 'error');
    return FALSE;
  }

  /* OK, now $xsl is a DOMDocument 
  // Save this DOMDocument as a temp file for debugging
  $temp = drupal_tempnam('temporary://', 'XSL_');
  $text = $xsl->saveXML();
  if (!file_put_contents($temp, $text)) {
    drupal_set_message("Could not save XSL as a temporary text file.", 'error');
    return FALSE;
  } else {
    drupal_set_message("Complete XSL transform saved as '$temp'.", 'status');
  } */

  // Find <variable>, the parent of all <item> nodes
  $dx = new DOMXPath($xsl);
  $nodes = $dx->query("/xsl:stylesheet/xsl:variable[@name='vPop']");
  if ($nodes->length < 1) {
    drupal_set_message("No <variable> element found in the XSL template.", 'errror');
    return FALSE;
  }
  
  $variable = $nodes->item(0);                      // the first (and only!) 'variable' element

  // Loop $i through $headers and $row.  Append each $header (xpath) and $row (data) as
  // an 'item' onto $top.  An example 'item': <item path="/mods/name[1]/role/roleTerm[@type='text']">creator</item>
  $length = min(count($headers), count($row));
  for ($i=0; $i<$length; $i++) {
    $item = $xsl->createElement('item',$row[$i]);   // DOMElement
    $path = $xsl->createAttribute('path');          // DOMAttribute
    $path->value = $headers[$i];                    // the path (attribute value)
    $item->appendChild($path);                      // apply the path attribute to the item
    $variable->appendChild($item);                  // append the new item to the parent $variable element
  }

  // OK, now $xsl is a DOMDocument containing our complete transform
  // Save this DOMDocument as a temp file for debugging
  $temp = drupal_tempnam('temporary://', 'XPathsToXML_');
  $text = $xsl->saveXML();
  if (!file_put_contents($temp, $text)) {
    drupal_set_message("Could not save XSL as a temporary text file.", 'error');
    return FALSE;
  } else {
    drupal_set_message("Complete XSL transform saved as '$temp'.", 'status');
  }

  // Prep to transform using the new XSL
  $xslt = new XSLTProcessor( );
  $xslt->importStyleSheet($xsl);

  // Fetch the MODS template
  if (!$template = file_get_contents($path.$MODS)) {
    drupal_set_message("Could not open '$MODS'.", 'error');
    return FALSE;
  }

  // Load the template as XML
  $xml = new DOMDocument( );
  if (!$xml->load($template)) {
    drupal_set_message("Could not load '$MODS' as XSL.", 'error'); 
    return FALSE;
  }
  
  // Apply $xslt to the MODS content
  $document = DOMDocument::loadXml($template);
  if ($doc = $xslt->transformToDoc($document)) {
    $xmlString = $doc->saveXML();
    $new_xml = new DOMDocument( );
    $new_xml->setContentFromString($xmlString);
  }

  return $xmlString;
}

/* http://php.net/manual/en/function.explode.php */
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

function make_node($doc, $parent, $element, $attribute, $value=NULL) {
  $node = $doc->createElement($element, $value);   // create a new $element $node
  if ($attribute) {
    $node->appendChild($attribute);     // attach $attribute to the new $node
  }
  $node = $parent->appendChild($node);  // attach the new $node to $parent and return it
  return $node;
}

/* http://stackoverflow.com/questions/834303/startswith-and-endswith-functions-in-php */
function startsWith($haystack, $needle) {
  // search backwards starting from haystack length characters from the end
  return $needle === "" || strrpos($haystack, $needle, -strlen($haystack)) !== false;
}

// My attempt to reproduce 
// private XmlNode GenerateNodeFromXPath(XmlDocument doc, XmlNode parent, string xpath) in PHP...
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
    "/mods/name[1][@type='personal']/namePart[@type='given']",
    "/mods/name[2][@type='personal']/namePart[@type='family']",
    "/mods/originInfo/dateCreated" );
  foreach( $xpaths as $xpath ) {
    $parent = GenerateNodeFromXPath($xml,NULL,$xpath,$xpath);
  }

  $temp = drupal_tempnam('temporary://', 'XML_');
  $text = $xml->saveXML();
  if (!file_put_contents($temp, $text)) {
    drupal_set_message("Could not save XML as a temporary text file.", 'error');
    return;
  } else {
    drupal_set_message("Complete XML saved as '$temp'.", 'status');
  }

}