<?php
/**
 * Utilities for ICG CSV Batch.
 * br2490
 */

/**
 * @param IslandoraFedoraObject $collection
 *
 * @return array
 *   An array describing each model allowed in this collection
 *   - pid: The PID with which the content model object.
 *     - pid: The PID with which the content model object.
 *     - name: The default label to use for new child objects of this type.
 *     - namespace: The default namespace for new child objects of this type.
 *
 * @throws Exception
 */
function _icg_csv_import_get_available_ingest_models(
  IslandoraFedoraObject $collection) {

  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'xml_form_builder', 'includes/associations');

  $all_content_models = islandora_get_content_models();

  if (!($policy = $collection['COLLECTION_POLICY'])) {
    throw new Exception('No collection policy was found for this collection.
     Please contact your site admin.');
  }

  $policy = new CollectionPolicy($policy->content);
  $policy_content_models = $policy->getContentModels();

  $ret = array();
  foreach ($policy_content_models as $available_model) {

    // We'll use the pid as the key for our array.
    $pid = $available_model['pid'];

    // Build our return array, keyed by $pid.

    // This provides the human readable name of the cModel.
    $ret['models'][$pid] = $all_content_models[$pid]['label'];

    // This provides the namespace.
    $ret['namespaces'][$pid] = $available_model['namespace'];

    // Grab all associated forms, regardless of DSID.
    $associations = xml_form_builder_get_associations(
      array(), $available_model, array(), FALSE);

    // Build an array of forms, though I think we're only using MODS (?).
    foreach ($associations as $association) {
      $ret['forms'][$pid][] = $association;
    }
  }

  return $ret;
}

/**
 * Small callback for ajax to get associated forms.
 *
 * @param $selected_cmodel
 * @param $xml_forms
 *
 * @return array
 */
function _icg_ajax_callback_xml_form($selected_cmodel, $xml_forms) {
  $ret = array();

  $cmodel_forms = $xml_forms[$selected_cmodel];

  foreach ($cmodel_forms as $form_name) {
    // Do a check here "['enabled']" should be TRUE.
    $name = $form_name['form_name'];
    $ret[$name] = $name;
  }

  return $ret;
}

/**
 * Leaving this here in case it needs to be a SPARQL query.
 *
 * STUB - silly function but here in case this needs to be more robust. Right
 * now we're relying completely on what the COLLECTION_POLICY dictates about
 * namespace; Either we'll need to traverse *->RELS_EXT to find out
 * about all the namespaces... or just SPARQL bright.
 *
 * @param $selected_cmodel
 * @param $namespaces
 *
 * @return array
 */
function _icg_ajax_callback_ns_form($selected_cmodel, $namespaces) {
  $ret = array();

  $ns = $namespaces[$selected_cmodel];
  $ret[$ns] = $ns;

  return $ret;
}

/**
 * Iterates through a form array and replaces specific fields and their options.
 *
 * @param $form - The metadata form as an object.
 * @param array $array - The metdata form as an array.
 * @param array $mappable_options - The vector of CSV data to map.
 * @param string $form_name - The metadata form name (or NULL if called from within).
 * 
 * @return array - Corresponding XPaths.  @TODO
 */
function _icg_form_array_iterator_set_options(&$form, &$array, $mappable_options, $form_name=NULL) {
  static $xpaths = array( );
  static $position;
  
  static $manipulable_fields = array(
    'textfield',
    'textarea',
    'tags',
  );

  // On the first call to this function $form_name should be provided, so use it
  // to fetch all possible xpaths. 
  if (isset($form_name)) {
    $xpaths = _icg_csv_import_get_xpaths($form_name, $array);
    $position = 0;
  } 
  
  /* @TODO 
   * There is a disconnect here between $xpaths and $form and I haven't been
   * able to determine how to associate elements of $form with their corresponding
   * xpath representations.  Simply tracking $position does not work since there's
   * not a one-to-one correspondence between $xpaths and elements.  For example, $xpaths
   * contains:
   * 
   *    /mods/titleInfo
   *    /mods/titleInfo/title
   * 
   * But /mods/titleInfo is NOT represented by any $form element because only its child,
   * /mods/titleInfo/title is.
   * 
   */
    
  // If this branch of the form is a manipulable field...
  if (isset($array['#type']) && in_array($array['#type'], $manipulable_fields)) {
    $array['#type'] = 'select';
    $array['#options'] = $mappable_options;
    $array['#description'] = 'This field is mapped to postion ' . ++$position . 
      ' with an XPath of: ' . $xpaths[$position] . '.';
    $title = $array['#title'];

    // Add a field for addition of an optional constant.
    $array[$title . '- constant'] = array(
      '#title' => $title . '-CONST',
      '#description' => 'Enter your constant value for this field',
      '#type' => 'textfield',
      '#states' => array(
        'invisible' => array(
          'input[name='.$title.']' => array('value' => 'USE_CONSTANT',),
          ),
        ),
      );
  }
  
  // This branch of the form (tree) is not a manipulable field, break it down further
  else if (is_array($array)) {
    foreach($array as $key => &$value){
      if (isset($value) && is_array($value)) {
        _icg_form_array_iterator_set_options($form, $value, $mappable_options);
      }
    }
  }
  
  return;
}


// This is a modified copy of function xml_form_builder_get_title_options($form_name) 
function _icg_csv_import_get_xpaths($form_name) {
  if (isset($form_name)) {
    module_load_include('inc', 'xml_form_builder', 'XMLFormRepository');
    $doc = XMLFormRepository::Get($form_name);  // This is a DOMDocument representation of the form

    if ($doc) {
      $xpath = new DOMXPath($doc);
      $result = $xpath->query('//element[@name]');
      // $result = $xpath->query('//element[@name] | //@*');   // failed attempt to get attributes
      $elements = dom_node_list_to_array($result);   // a mix of DOMElement and DOMAttr nodes
      if (count($elements) > 0) {
        $output = array( );
        foreach ($elements as $element) {
          $result = $xpath->query('ancestor-or-self::node()[@name]', $element);
          $ancestors = dom_node_list_to_array($result);
          $ancestors_array = array();
          if (count($ancestors) > 0) {
            foreach ($ancestors as $ancestor) {
              $name = $ancestor->getAttribute('name');
              $ancestors_array[] = (string) $name;
            }
          }
          $x = "/mods/" . implode('/', $ancestors_array);
          $output[] = str_replace('/0', '[0]', $x);
        }
        return $output;
      }
    }
  }
  return array(NULL => t('Metadata Form Not Found!'));
}


// a DOMElement

