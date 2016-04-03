<?php


/**
 * CSV Import Form Callback
 */
function icg_csv_import_form($form, &$form_state, $collection) {
  dpm($form_state);

  if (!isset($form_state['values'])) {
    $form_state['step'] = 0;
    $form_state['all_values'] = array();
  }

  // Build the form parts in 'steps'
  $form_content = icg_csv_import_get_form($collection);

  // Grab the current 'step'
  $current_step = $form_state['step'];

  //Return the current page for the step
  return $form_content[$current_step];

}

/**
 * CSV Import Form Submit
 */
function icg_csv_import_form_submit($form, &$form_state) {
   // Save our values
  $form_state['all_values'] += $form_state['values'];

  if ($form_state['step'] < 2) {
    // Do not submit the form
    $form_state['rebuild'] = TRUE;
    // Instead go to the next page.
    $form_state['step']++;
  } else {
    dpm($form_state);
    // do something with all_values...
  }
  
}

/**
 * @param $collection
 * @return array
 */
function icg_csv_import_get_form($collection) {
  $form = array();
  module_load_include('inc', 'icg_csv_import', 'includes/utilities');

  $models_and_namespaces = _icg_get_available_models_namespaces_for_form($collection);

  $step1['content_model'] = array(
    '#type' => 'select',
    '#title' => t('Destination content model'),
    '#description' => t('Select an available content model to ingest into.'),
    '#options' =>  $models_and_namespaces['models'],
  );

  $step1['namespace'] = array(
    '#type' => 'select',
    '#title' => t('Destination namespace'),
    '#description' => t('Select an available destination namespace'),
    '#options' => $models_and_namespaces['namespaces'],
  );

  $step1['csv_delimiter'] = array (
    '#type' => 'textfield',
    '#title' => t('Field Delimiter'),
    '#description' => t('Enter the character that separates fields in your CSV file.'),
    '#size' => 10,
    '#default_value' => ',',
  );

  $step1['csv_file'] = array (
    '#type' => 'file',
    '#title' => t('Choose a file'),
    '#size' => 22,
  );

  $step1['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next'),
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
