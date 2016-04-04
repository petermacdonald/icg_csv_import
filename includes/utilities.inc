<?php
/**
 * Utilities for ICG CSV Batch.
 * br2490
 */

/**
 * @param IslandoraFedoraObject $collection
 * @return array
 */
function _icg_csv_import_get_available_models(IslandoraFedoraObject $collection) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $all_content_models = islandora_get_content_models();

  $policy = new CollectionPolicy($collection['COLLECTION_POLICY']->content);
  $policy_content_models = $policy->getContentModels();

  $ret = array();

  foreach ($all_content_models as $pid => $content_model) {

    // Skip unused/unavailable cModels.
    if (!($content_model_in_policy = isset($policy_content_models[$pid]))) {
      continue;
    }

    $ret[$pid] = $content_model['label'];

//    $namespace = $policy_content_models[$pid]['namespace'];
//
//    $ret[$pid] = array (
//      $pid => $content_model['label'], // cmodel pid => human readable label
//      $namespace => $namespace, // namespace => namespace... (#unecessary comment)
//    );
  }

  return $ret;
}


function _icg_csv_import_get_available_namespaces(IslandoraFedoraObject $collection, $pid) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $policy = new CollectionPolicy($collection['COLLECTION_POLICY']->content);
  $policy_content_models = $policy->getContentModels();

  $namespace = $policy_content_models[$pid]['namespace']; // CAN THERE BE MORE THAN ONE?

  $ret = array();
  $ret[$namespace] = $namespace;

  return $ret;
}



/**
 * @param array $islandora_cmodels
 * @return array where id = form_name
 */
function _icg_csv_import_get_cmodel_associations(Array $islandora_cmodels) {
  module_load_include('inc', 'xml_form_builder', 'includes/associations');

  $associations = xml_form_builder_get_associations(array(), $islandora_cmodels, array(), FALSE);

  $ret = array ();
  foreach ($associations as $association) {
    $form_name = $association['form_name'];
    $ret[$form_name] = $form_name;
  }

  return $ret;
}

// @DEPRECATED or UNAPPRECIATED?? @TODO look at this more and figure out what you were trying to do...
//function _icg_csv_import_get_form_part1(IslandoraFedoraObject $collection) {
//  module_load_include('inc', 'islandora', 'includes/utilities');
//  module_load_include('inc', 'xml_form_builder', 'includes/associations');
//
//  $all_content_models = islandora_get_content_models();
//
//  $policy = new CollectionPolicy($collection['COLLECTION_POLICY']->content);
//  $policy_content_models = $policy->getContentModels();
//
//  $cmodel_names_forms = array();
//
//  foreach ($all_content_models as $pid => $content_model) {
//
//    // Skip unused/unavailable cModels.
//    if (!($content_model_in_policy = isset($policy_content_models[$pid]))) {
//      continue;
//    }
//
//    // Gather XML form associations.
//    $associations = xml_form_builder_get_associations(array(), array($pid), array(), FALSE);
//
//    $cmodel_names_forms[$pid] = array (
//      'namespace' => $policy_content_models[$pid]['namespace'],
//      'content_model_label' => $content_model['label'],
//      'associated_forms' => $associations,
//    );
//  }
//
//  return $cmodel_names_forms;
//}