<?php


/**
 * CSV Import Form Callback
 */
function icg_csv_import_form($form, &$form_state, $collection) {

  if (isset($form_state['values'])) {
    $currstep = $form_state['step'] + 1;
  }
  else {
    $currstep = 0;
  }

  $form_state['step'] = $currstep;
  $allsteps = icg_csv_import_get_form($collection);
  $currform = $allsteps[$currstep];

  return $currform;

}


function icg_csv_import_get_form($collection) {
  $form = array();


  module_load_include('inc', 'islandora_solution_pack_collection', 'includes/utilities');

  $step1['cm_select'] = array(
    '#type' => 'select',
    '#title' => t('Destination content model'),
    '#description' => t('Select a content model to ingest into.'),
    '#options' => islandora_basic_collection_get_content_models_as_form_options(islandora_get_content_models()),
  );

  $step1['namespace'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination namespace'),
    // TODO: Get namespace, don't use whole PID of the collection
    '#default_value' => $collection->id,
  );

  $form[] = $step1;

  // If $form state shows content model selected...
  // Need to get this xml datastreams available for the selected content model
  $datastreams = array();

  $step2['datastream'] = array(
    '#type' => 'select',
    '#title' => t('Select destination xml datastream'),
    '#options' => array(),

  );

   $step2['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Next')
  );

  $form[] = $step2;

  $step3['birthdate'] = array(
    '#type' => 'date',
    '#title' => t('Birthdate'),
  );

  $step3['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit')
  );

  $form[] = $step3;

  return $form;
}

/**
 * CSV Import Form Submit Callback
 */
