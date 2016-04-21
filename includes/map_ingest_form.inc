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

  // Build the form steps
  try {
    $step = $form_state['step'];
    $current_form = icg_csv_import_get_form($step, $collection,
      $form_state);
  } catch (Exception $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    return FALSE;
  }

  // Return the current page for the step
  return $current_form;
}

/**
 * Implements hook_form_submit().
 *
 * @param $form
 * @param $form_state
 */
function icg_csv_import_form_submit($form, &$form_state) {
  // Save our current values to all_values.
  $form_state['all_values'] += $form_state['values'];
  dpm($form_state);

  // CSV file handler. I'm sure this could be done better.
  if (isset($form_state['values']['csv_file'])) {

    // File validation, upload and storage in temporary://.
    $validators = array(
      'file_validate_extensions' => array('csv txt'),
      // @TODO set a file size limit?
      // @TODO set mime/type?
    );

    $file =
      file_save_upload('csv_file', $validators, FALSE, FILE_EXISTS_REPLACE);
    if ($file) {
      // drupal file object
      $form_state['all_values']['csv_file'] = $file;
    }
    else {
      form_set_error('csv_file', "A CSV file is required to continue.");
    }
  }

  // Form multi-part 'handler'
  if ($form_state['step'] < 2) {
    // Do not submit the form
    $form_state['rebuild'] = TRUE;
    // Instead go to the next page.
    $form_state['step']++;
  }
  else {
    dpm($form_state);
    // do something with $form_state[all_values] (drupal batch?)
    // redirect away from form.
  }
}




/**
 * @param $form
 * @param $form_state
 * @return array
 */
function icg_csv_import_form_ajax_callback($form, &$form_state) {
  $commands = array();
  $commands[] = ajax_command_replace('#xml-dropdown-update',
    drupal_render($form['xml_form']));
  $commands[] = ajax_command_replace('#namespace-dropdown-update',
    drupal_render($form['namespace']));
  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * @param $step
 * @param $collection
 * @param $form_state
 *
 * @return array
 * @throws \Exception
 */
function icg_csv_import_get_form($step, $collection, &$form_state) {
  $form = array();

  if (!isset($form_state['collection_policy'])) {
    module_load_include('inc', 'icg_csv_import', 'includes/utilities');

    // This will throw an error if no CPOLICY.
    $form_state['collection_policy'] =
      _icg_csv_import_get_available_ingest_models($collection);
  }

  switch ($step):
    case 0:
      module_load_include('inc', 'icg_csv_import', 'includes/utilities');

      // This is unnecessary but it feels right.
      $content_models = $form_state['collection_policy']['models'];
      $namespaces = $form_state['collection_policy']['namespaces'];
      $xml_forms = $form_state['collection_policy']['forms'];

      $selected_cmodel = isset($form_state['values']['content_model']) ?
        $form_state['values']['content_model'] : key($content_models);

      $form['content_model'] = array(
        '#type' => 'select',
        '#title' => t('Destination Content Model'),
        '#description' => t('Select an available content model to ingest into.'),
        '#options' => $content_models,
        '#default_value' => $selected_cmodel,
        '#ajax' => array(
          'callback' => 'icg_csv_import_form_ajax_callback',
        ),
      );

      $form['xml_form'] = array(
        '#type' => 'select',
        '#title' => t('Select XML form to use...'),
        '#prefix' => '<div id="xml-dropdown-update">',
        '#suffix' => '</div>',
        '#description' => t('Lorem ipsum.'),
        '#options' => _ajax_callback_xml_form($selected_cmodel, $xml_forms),
      );

      $form['namespace'] = array(
        '#type' => 'select',
        '#title' => t('Destination Namespace'),
        '#prefix' => '<div id="namespace-dropdown-update">',
        '#suffix' => '</div>',
        '#description' => t('Select an available destination namespace'),
        '#options' => _ajax_callback_ns_form($selected_cmodel, $namespaces),
      );

      $form['csv_delimiter'] = array(
        '#type' => 'textfield',
        '#title' => t('CSV Field Delimiter'),
        '#description' => t('Enter the character that separates fields in your CSV file.'),
        '#size' => 10,
        '#required' => TRUE,
        '#default_value' => ',',
      );

      $form['ingest_as_inactive'] = array(
        '#type' => 'checkbox',
        '#title' => t('Ingest items as inactive?'),
        '#default_value' => TRUE,
      );

      $form['csv_file'] = array(
        '#name' => 'files[csv_file]',
        '#type' => 'file',
        '#title' => t('Select your CSV File.'),
        '#description' => t('Allowed file types are .csv and .txt'),
        '#size' => 22,
        '#default_value' => TRUE,
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next'),
      );

      break;

    case 1:

      module_load_include('inc', 'icg_csv_import', 'includes/csv');
      $file = $form_state['all_values']['csv_file'];
      $delimit = $form_state['all_values']['csv_delimiter'];

      // @TODO: ask JDP if this is correct CSV.inc use.
      $csv_prepared = icg_csv_load_csv_data($file->uri, $delimit);
      $csv_parsed = icg_csv_parse_csv_data($csv_prepared);

      $mappable_headers = $csv_parsed[0];

      dpm($mappable_headers);
//      foreach($mappable_headers as $header) {
//        $form['header'][$header] = array(
//          '#type' => 'select',
//          '#title' => t('Words...'),
//          '#options' => array($header),
//        );
//      }

      module_load_include('inc', 'xml_form_api', 'XMLFormDefinition');
      module_load_include('inc', 'xml_form_builder', 'XMLFormRepository');

      $definition =
        new XMLFormDefinition(XMLFormRepository::Get($form_state['all_values']['xml_form']));
      $node = $definition->get()->documentElement;

      var_dump($node);
//      $child = $node->childNodes;
//      foreach($child as $item) {
//        if ($item->nodeType == XML_TEXT_NODE) {
//          dpm($item->value);
//        }
//        else {
//          if ($item->nodeType == XML_ELEMENT_NODE) {
//            dpm($item->nodeName);
//          }
//        }
//      }


      // If $form state shows content model selected...
      // Need to get this xml datastreams available for the selected content model
      $datastreams = array();

      $form['datastream'] = array(
        '#type' => 'select',
        '#title' => t('Select destination xml datastream'),
        '#options' => array(),
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next')
      );

      $form[] = $form;
      break;

    case 2:

      $step3['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit')
      );

      $form[] = $step3;
      break;
  endswitch;

  return $form;
}
