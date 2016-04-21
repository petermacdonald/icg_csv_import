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
function _ajax_callback_xml_form($selected_cmodel, $xml_forms) {
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
function _ajax_callback_ns_form($selected_cmodel, $namespaces) {
  $ret = array();

  $ns = $namespaces[$selected_cmodel];
  $ret[$ns] = $ns;

  return $ret;
}

