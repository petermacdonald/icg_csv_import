<?php

/**
 * CSV Import Form Callback
 */
function icg_csv_import_form($collection) {
  $form = array();

  $fun = module_load_include('inc', 'islandora_solution_pack_collection', 'includes/utilities');
  dpm($fun);

  $form['cm_select'] = array(
    '#type' => 'select',
    '#title' => t('Destination content model'),
    '#description' => t('Select a content model to ingest into.'),
    '#options' => islandora_basic_collection_get_content_models_as_form_options(islandora_get_content_models()),
  );



  return render($form);
}


//islandora_get_content_models()

/**
 * CSV Import Form Submit Callback
 */
