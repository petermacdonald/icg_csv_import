<?php

/**
 * CSV Import Form Callback
 * @param $form
 * @param $form_state
 * @param $collection
 * @return mixed
 */
function icg_csv_import_form($form, &$form_state, $collection) {

  // If this is the first time the user hits the form page:
  if (!isset($form_state['values'])) {
    $form_state['step'] = 0;
    $form_state['all_values'] = array();
  }

  form_set_error(NULL, 'THE BEES');
  // Build the form steps
  try {
    $form_content = icg_csv_import_get_form($collection, $form_state);
  } catch (Exception $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    return;
  }

  // Grab the current step
  $current_step = $form_state['step'];

  // Return the current page for the step
  return $form_content[$current_step];
}

/**
 * Implements hook_form_submit
 * @param $form
 * @param $form_state
 */
function icg_csv_import_form_submit($form, &$form_state) {
   // Save our current values to all_values.
  $form_state['all_values'] += $form_state['values'];

  // CSV file handler. I'm sure this could be done better.
  if (isset($form_state['values']['csv_file'])) {

    // File validation, upload and storage in temporary://.
    $validators = array(
      'file_validate_extensions' => array('csv txt'),
      // @TODO set a file size limit?
    );

    $file = file_save_upload('csv_file', $validators, FALSE, FILE_EXISTS_REPLACE);
    if ($file) {
      $form_state['all_values']['csv_file'] = $file; // drupal file object
    }
    else{
      form_set_error('csv_file', "A CSV file is required to continue.");
    }
  }

  // Form multi-part handler
  if ($form_state['step'] < 2) {
    // Do not submit the form
    $form_state['rebuild'] = TRUE;
    // Instead go to the next page.
    $form_state['step']++;
  }
  else {
    dpm($form_state);
    // do something with $form_state[all_values] (drupal batch?) redirect away from form.
  }

}

/**
 * @param $form
 * @param $form_state
 * @return array
 */
function icg_csv_import_form_ajax_callback($form, &$form_state) {
  $commands = array();
  $commands[] = ajax_command_replace('#xml-dropdown-update', drupal_render($form['xml_form']));
  $commands[] = ajax_command_replace('#namespace-dropdown-update', drupal_render($form['namespace']));
  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * @param $collection
 * @param $form_state
 * @return array
 */
function icg_csv_import_get_form($collection, &$form_state) {
  module_load_include('inc', 'icg_csv_import', 'includes/utilities');

  $form = array();

  $content_models = _icg_csv_import_get_available_models($collection); // This will throw an error if no CPOLICY.

  $selected_cmodel = isset($form_state['values']['content_model']) ? $form_state['values']['content_model'] : key($content_models);

  $step1['content_model'] = array(
    '#type' => 'select',
    '#title' => t('Destination Content Model'),
    '#description' => t('Select an available content model to ingest into.'),
    '#options' =>  $content_models, // as 'cmodel pid' = 'cmodel name'
    '#default_value' => $selected_cmodel,
    '#ajax' => array (
      'callback' => 'icg_csv_import_form_ajax_callback',
    ),
  );

  $step1['xml_form'] = array(
    '#type' => 'select',
    '#title' => t('Select XML form to use...'),
    '#prefix' => '<div id="xml-dropdown-update">',
    '#suffix' => '</div>',
    '#description' => t('Lorem ipsum.'),
    // options as 'form name' = 'form name' (because this is how we need to grab them from the xml db func)
    '#options' => _icg_csv_import_get_cmodel_associations(array($selected_cmodel)),
  );

  $step1['namespace'] = array(
    '#type' => 'select',
    '#title' => t('Destination Namespace'),
    '#prefix' => '<div id="namespace-dropdown-update">',
    '#suffix' => '</div>',
    '#description' => t('Select an available destination namespace'),
    '#options' => _icg_csv_import_get_available_namespaces($collection, $selected_cmodel),
  );

  $step1['csv_delimiter'] = array (
    '#type' => 'textfield',
    '#title' => t('CSV Field Delimiter'),
    '#description' => t('Enter the character that separates fields in your CSV file.'),
    '#size' => 10,
    '#required' => TRUE,
    '#default_value' => ',',
  );

  $step1['ingest_as_inactive'] = array(
    '#type' => 'checkbox',
    '#title' => t('Ingest items as inactive?'),
    '#default_value' => TRUE,
  );

  $step1['csv_file'] = array (
    '#name' => 'files[csv_file]',
    '#type' => 'file',
    '#title' => t('Select your CSV File.'),
    '#description' => t('Allowed file types are .csv and .txt'),
    '#size' => 22,
    '#default_value' => TRUE, // Ayyyyy, don't worry about it!
  );

  $step1['submit'] = array(
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
