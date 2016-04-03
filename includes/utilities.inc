<?php
/**
 * Utilities for ICG CSV Batch.
 * br2490
 */

/**
 * _icg_get_available_models_namespaces_for_form
 * Loads collection policy and builds arrays of available cModels and Namespaces.
 * @param IslandoraFedoraObject $collection
 * @return array for display on form
 */
function _icg_get_available_models_namespaces_for_form(IslandoraFedoraObject $collection) {
  module_load_include('inc', 'islandora', 'includes/utilities');
//  module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');
  $all_content_models = islandora_get_content_models();
  $default_namespace = drupal_substr($collection->id, 0, strpos($collection->id, ":"));

  $policy = new CollectionPolicy($collection['COLLECTION_POLICY']->content);
  $policy_content_models = $policy->getContentModels();

  $available_collection_models = array();
  $available_namespaces = array();
  foreach ($all_content_models as $pid => $content_model) {

    // Skip unused/unavailable cModels.
    if (!($content_model_in_policy = isset($policy_content_models[$pid]))) {
      continue;
    }

    $namespace = $content_model_in_policy ? $policy_content_models[$pid]['namespace'] : $default_namespace;
    $available_collection_models[$pid] = $content_model['label'];
    $available_namespaces[$namespace] = $namespace; // Overwrite existing namespaces.
  }

  $ret = array (
    'models' => $available_collection_models,
    'namespaces' => $available_namespaces,
  );

  return $ret;
}