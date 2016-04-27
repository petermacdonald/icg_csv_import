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
    $current_form = icg_csv_import_get_form($step, $collection, $form_state);
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
  module_load_include('inc', 'icg_csv_import', 'includes/batch');

  // Save our current values to all_values.
  $form_state['all_values'] += $form_state['values'];
  # dpm($form_state);

  switch ($form_state['step']):
    case 0:
      // Save our current values to all_values.
      $form_state['all_values'] += $form_state['values'];
      // Do not submit the form
      $form_state['rebuild'] = TRUE;
      // Instead go to the next page.
      $form_state['step']++;
      break;
    case 1:
      // Save our crosswalk to all_values crosswalk.
      $form_state['all_values']['crosswalk'] = $form_state['values'];
      // Do not submit the form
      $form_state['rebuild'] = TRUE;
      // Instead go to the next page.
      $form_state['step']++;
      break;
    case 2:
      // Do something with all_values and redirect away from the form!
      icg_csv_import_build_batch($form_state);
      dpm($form_state);
      break;

  endswitch;
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

  // Since this is used frequently in here, we include it universally for each
  // form part.
  module_load_include('inc', 'icg_csv_import', 'includes/utilities');

  if (!isset($form_state['collection_policy'])) {

    // This will throw an error if no CPOLICY.
    $collection_info =
      _icg_csv_import_get_available_ingest_models($collection);

    // Keep a copy in the form_state, we may need it later, else remove.
    $form_state['collection_policy'] = $collection_info;

    // Save parent PIDs.
    $form_state['all_values']['parent_pid'] = $collection->id;
  }

  // If $form_state['values']['skip_field_mapping'] is TRUE,
  // then skip $step 1 (form #1) and go straight to form #2.
  // if (isset($form_state['values']['skip_field_mapping'])) {
  //  if ($form_state['values']['skip_field_mapping'] && $step === 1) {
  //    $form_state['step'] = ++$step;
  //  }
  // }

  // Switch through the form $steps.
  switch ($step):

    case 0:    // The first form page... collect all control parameters.

      // This is unnecessary but it feels right.
      $content_models = $form_state['collection_policy']['models'];
      $namespaces = $form_state['collection_policy']['namespaces'];
      $xml_forms = $form_state['collection_policy']['forms'];

      $selected_cmodel = isset($form_state['values']['content_model']) ?
        $form_state['values']['content_model'] : key($content_models);

      $form['#validate'][] = 'icg_csv_import_validate';

      $form['content_model'] = array(
        '#type' => 'select',
        '#title' => t('Destination Content Model'),
        '#description' => t('Select an available content model to ingest into.'),
        '#options' => $content_models,
        '#default_value' => $selected_cmodel,
        '#required' => TRUE,
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
        '#required' => TRUE,
        '#options' => _icg_ajax_callback_xml_form($selected_cmodel, $xml_forms),
      );

      $form['namespace'] = array(
        '#type' => 'select',
        '#title' => t('Destination Namespace'),
        '#prefix' => '<div id="namespace-dropdown-update">',
        '#suffix' => '</div>',
        '#description' => t('Select an available destination namespace'),
        '#required' => TRUE,
        '#options' => _icg_ajax_callback_ns_form($selected_cmodel, $namespaces),
      );

      $form['csv_delimiter'] = array(
        '#type' => 'textfield',
        '#title' => t('CSV Field Delimiter'),
        '#description' => t('Enter the character that separates fields in 
        your CSV file.'),
        '#size' => 10,
        '#required' => TRUE,
        '#default_value' => ',',
      );

      $form['csv_file'] = array(
        '#name' => 'files[csv_file]',
        '#type' => 'file',
        '#title' => t('Select your CSV File.'),
        '#description' => t('Allowed file types are .csv and .txt'),
        '#size' => 200,
        '#required' => TRUE,
        '#default_value' => TRUE,
      );

      $form['skip_field_mapping'] = array(
        '#type' => 'checkbox',
        '#title' => t('CSV first row already contains the field mapping?'),
        '#description' => t('Check this box if the first row of your CSV file already contains mapped XPaths.'),
        '#default_value' => FALSE,
      );

      $form['credentials_username'] = array(
        '#type' => 'textfield',
        '#title' => t('File Transfer Username'),
        '#description' => t('Enter the username required to execute content file transfers.'),
        '#size' => 20,
        '#required' => FALSE,
        '#default_value' => 'None',
      );

      $form['credentials_password'] = array(
        '#type' => 'password',
        '#title' => t('File Transfer Password'),
        '#description' => t('Enter the password corresponding to username (above) to execute content file transfers.'),
        '#size' => 20,
        '#required' => FALSE,
        '#default_value' => 'None',
      );

      $form['ingest_as_inactive'] = array(
        '#type' => 'checkbox',
        '#title' => t('Ingest items as inactive?'),
        '#default_value' => TRUE,
      );

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next'),
      );

      break;

    case 1:     // Form page 2... Map CSV columns to metadata form elements.
      
      // A metadata form has been selected.  Fetch it's title and add it's data array to
      // $form_state['collection_policy']['selected']
      $selected_form = $form_state['all_values']['xml_form'];
      $cmodel = $form_state['all_values']['content_model'];
      $forms = $form_state['collection_policy']['forms'][$cmodel];
      foreach($forms as $fdata) {
        if ($fdata['form_name'] === $selected_form) {
          $form_state['collection_policy']['selected'] = $fdata;
          $path = drupal_get_path('module', 'xml_form_builder');
          $form_state['all_values']['transform'] = $path.'/transforms/'.$fdata['transform'];
          $label_field = '/mods';
          foreach($fdata['title_field'] as $tfield) {
            $label_field .= '/'.$tfield;
          }
          $form_state['all_values']['label_field'] = $label_field;
          break;
        }
      }  
      
      // No mapping necessary...let the user know to proceed.
      if ($form_state['values']['skip_field_mapping']) {

        $form['message'] = array(
          '#markup' => "<p>You have indicated that metadata XPaths are already mapped in the first row of the CSV file.  Click 'Next' to proceed.</p>",
        );

      } else {    // Map CSV columns to metadata form elements.

        // Load our CSV parser.
        module_load_include('inc', 'icg_csv_import', 'includes/csv');

        // Set some variables we know we'll end up using.
        $file = $form_state['all_values']['csv_file'];
        $delimit = $form_state['all_values']['csv_delimiter'];
        $form_name = $form_state['all_values']['xml_form'];

        // Parse our CSV file.
        // @TODO: ask JDP if this is correct CSV.inc use.
        $csv_prepared = icg_csv_load_csv_data($file->uri, $delimit);
        $csv_parsed = icg_csv_parse_csv_data($csv_prepared);

        // Create an array of options for our form. Include an 'ignore' option.
        // @TODO: check with JDP if this is acceptable use of CSV array[0].
        $mappable_options = array(NULL => 'Ignored Field',);
        foreach ($csv_parsed[0] as $column_heading) {
          $mappable_options[$column_heading] = $column_heading;
        }

        // Grab the XML form the user selected as associated.
        $xml_form = xml_form_builder_get_form($form, $form_state, $form_name);

        // Make the form OURS! Change all textareas/textfields into selects and
        // use our options array from above.
        // Stand clear of the crazy iterator while in motion.
        _icg_form_array_iterator_set_options($xml_form, $mappable_options);

        $form = $xml_form;
      }

      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Next')
      );

      break;

    case 2:     // What was this page here for? Something probably.  @TODO...maybe display the XPath map?
      
      $step3['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit')
      );

      $form[] = $step3;
      break;
    
  endswitch;

  return $form;
}

/**
 * Form validation handler.
 * 
 * @param $form
 * @param $form_state
 */
function icg_csv_import_validate($form, &$form_state) {
  $validators = array('file_validate_extensions' => array('csv txt'));

  // Check for a new uploaded CSV.
  $file = file_save_upload('csv_file', $validators);
  if (isset($file)) {
    // File upload was attempted.
    if ($file) {
      $form_state['values']['csv_file'] = $file;  // Put the temporary file in $form_values so we can save it on submit.
    } else {
      form_set_error('icg_csv_import', t('The CSV file could not be uploaded.'));  // File upload failed.
    }
  }
}