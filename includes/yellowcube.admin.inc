<?php

/**
 * @file
 * Contains the administrative pages and form callbacks for the YellowCube module.
 */

/**
 * Builds the form for configuring the YellowCube integration.
 */
function yellowcube_admin_settings_form($form, &$form_state) {

  $form['logo'] = array(
    '#prefix' => '<div>',
    '#markup' => theme('image', array('path' => drupal_get_path('module', 'yellowcube') . '/img/logo.jpg')),
    '#suffix' => '</div>',
  );

  $form['yellowcube_sender'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube sender'),
    '#description' => t('Your YellowCube Shop ID number.'),
    '#default_value' => variable_get('yellowcube_sender', ''),
  );

  $form['yellowcube_depositor'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube depositor number'),
    '#description' => t('ID number assigned to your business by YellowCube.'),
    '#default_value' => variable_get('yellowcube_depositor', ''),
  );

  $form['yellowcube_partner'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube partner number'),
    '#description' => t('ID number assigned to your business by YellowCube.'),
    '#default_value' => variable_get('yellowcube_partner', ''),
  );

  $form['yellowcube_plant'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube plant ID'),
    '#default_value' => variable_get('yellowcube_plant', ''),
  );

  $form['yellowcube_tara_factor'] = array(
    '#type' => 'textfield',
    '#title' => t('YellowCube tara factor'),
    '#default_value' => variable_get('yellowcube_tara_factor', '1.1'),
  );

  $form['yellowcube_certificate'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to certificate'),
    '#description' => t('For security reasons, the certificate should be placed outside of the webroot.'),
    '#default_value' => variable_get('yellowcube_certificate', ''),
  );

  $form['yellowcube_password'] = array(
    '#type' => 'password',
    '#title' => t('Certificate password'),
    '#description' => t('Password phrase used together with the certificate.'),
    '#default_value' => variable_get('yellowcube_password', ''),
  );

  // T=Test / P=Production / D=Development.
  $form['yellowcube_mode'] = array(
    '#type' => 'select',
    '#title' => t('Operating mode'),
    '#options' => array(
      'P' => t('Production'),
      'T' => t('Test'),
  // Only used during development of YC Connector.
      'D' => t('Development'),
    ),
    '#default_value' => variable_get('yellowcube_mode', 'T'),
    '#description' => t('If test mode is enabled, orders will get sent to the YellowCube test system. No actual goods will be sent.'),
  );

  $form['yellowcube_endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('URL to the Endpoint'),
    '#description' => t('YellowCube SOAP EntryPoint'),
    '#default_value' => variable_get('yellowcube_endpoint', ''),
  );

  $form['yellowcube_status_label_received'] = array(
    '#type' => 'textfield',
    '#title' => t('Order status label: received'),
    '#description' => t('The order status label'),
    '#default_value' => variable_get('yellowcube_status_label_received', 'YellowCube: received by YellowCube'),
  );

  $form['yellowcube_status_label_sending'] = array(
    '#type' => 'textfield',
    '#title' => t('Order status label: sending'),
    '#description' => t('The order status label for pending orders'),
    '#default_value' => variable_get('yellowcube_status_label_sending', 'YellowCube: sending to YellowCube'),
  );

  $form['yellowcube_status_label_failed'] = array(
    '#type' => 'textfield',
    '#title' => t('Order status label: failed'),
    '#description' => t('The order status label for failed orders'),
    '#default_value' => variable_get('yellowcube_status_label_failed', 'YellowCube: sending to YellowCube failed'),
  );

  $form['yellowcube_log_level'] = array(
    '#type' => 'select',
    '#title' => t('Log level'),
    '#options' => array(
      'debug' => t('Debug – Log every SOAP request and response'),
      'info' => t('Info – Log YellowCube service method calls'),
      'error' => t('Error – Log SOAP faults'),
      '' => t('None – No logging'),
    ),
    '#description' => t(
      "Log messages into !log_messages. Levels include the ones below. Default is 'error'.",
      array('!log_messages' => l(t('log messages'), 'admin/reports/dblog'))
    ),
    '#default_value' => variable_get('yellowcube_log_level', 'error'),
  );
  $form = system_settings_form($form);

  // Add "Test connection" button to validate credentials.
  $form['actions']['connect'] = array(
    '#type' => 'submit',
    '#value' => t('Test connection'),
    '#submit' => array('_yellowcube_admin_test_connection'),
  );

  return $form;
}

/**
 * Submit function for "Test connection" button in the configuration form.
 */
function _yellowcube_admin_test_connection(array $form, array &$form_state) {
  // Do nothing as the submit callback will trigger configuration form
  // validation and test connection credentials.
}

/**
 * Validates whether a given value is a 10-digit number.
 *
 * @param string $value
 *   The string value to validate.
 *
 * @return bool
 *   Returns TRUE if the given value is a 10-digit number. Otherwise, FALSE.
 */
function yellowcube_admin_validate_ten_digit_number($value) {
  return (bool) preg_match('/^\d{10}$/', $value);
}

/**
 * Validates yellowcube_admin_settings_form().
 */
function yellowcube_admin_settings_form_validate($form, &$form_state) {
  // Ensure the sender contains less than 10 characters and the first upper case
  // character.
  if (!preg_match('/^(([A-Z0-9]{1}[a-z0-9]{1,9})|(YCTest)|(Yctest)|(YELLOWCUBE)|(LOGOS))$/', $form_state['values']['yellowcube_sender'])) {
    form_set_error('yellowcube_sender', t('The YellowCube sender may not be greater than 10 characters. Only the first letter should be capitalized. E.g. "Tradeshop".'));
  }

  // Ensure the depositor number is a 10-digit number.
  if (!yellowcube_admin_validate_ten_digit_number($form_state['values']['yellowcube_depositor'])) {
    form_set_error('yellowcube_depositor', t('The YellowCube depositor number should be a 10-digit number. E.g. "0000054321".'));
  }

  // Ensure the partner number is a 10-digit number.
  if (!yellowcube_admin_validate_ten_digit_number($form_state['values']['yellowcube_partner'])) {
    form_set_error('yellowcube_partner', t('The YellowCube partner number should be a 10-digit number. E.g. "0000300200".'));
  }

  // Ensure the plant ID is a 4 characters long.
  if (!preg_match('/^\w{4}$/', $form_state['values']['yellowcube_plant'])) {
    form_set_error('yellowcube_plant', t('The YellowCube plant ID should be 4 characters long. E.g. "Y004".'));
  }

  // Ensure the certificate file exists if it is set.
  if (!empty($form_state['values']['yellowcube_certificate']) && !file_exists($form_state['values']['yellowcube_certificate'])) {
    form_set_error('yellowcube_certificate', t('The provided certificate does not exist.'));
  }

  // Try to connect to YellowCube in case there are no validation errors.
  if (empty(form_get_errors())) {
    module_load_include('client.inc', 'yellowcube');

    if (yellowcube_client_validate_connection($form_state['values'])) {
      drupal_set_message(t('Connection to YellowCube successful.'));
    }
    else {
      form_set_error('', t('Could not connect to YellowCube service.'));
    }
  }
}

/**
 * YellowCube inventory Management.
 */
function yellowcube_inventory_form($form, &$form_state) {

  drupal_set_title(t('YellowCube inventory management'));

  $form['logo'] = array(
    '#prefix' => '<div>',
    '#markup' => theme('image', array('path' => drupal_get_path('module', 'yellowcube') . '/img/logo.jpg')),
    '#suffix' => '</div>',
  );

  $form['send_data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Send product data including physical dimensions (ART)'),
    '#weight' => 5,
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['send_data']['description'] = array(
    '#prefix' => '<div>',
    '#markup' => t('With this button, you can send all your product data to YellowCube. Make sure, you include all dimensions and weights in your product data.'),
    '#suffix' => '</div>',
  );

  $form['send_data']['send_product_data'] = array(
    '#type' => 'submit',
    '#name' => 'send_data',
    '#value' => t('Send all product data to YellowCube'),
    '#weight' => 10,
  );

  $form['fetch_data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Fetch product stock level data (BAR)'),
    '#weight' => 6,
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['fetch_data']['description'] = array(
    '#prefix' => '<div>',
    '#markup' => t('With this button, you get the current stock data.'),
    '#suffix' => '</div>',
  );

  $form['fetch_data']['get_inventory'] = array(
    '#type' => 'submit',
    '#name' => 'fetch_data',
    '#value' => t('Get current stock data'),
    '#submit' => array('yellowcube_inventory_form_submit'),
    '#weight' => 12,
  );

  return $form;

}

/**
 *
 */
function yellowcube_inventory_form_submit($form, &$form_state) {
  module_load_include('client.inc', 'yellowcube');

  if ($form_state['clicked_button']['#name'] == 'fetch_data') {
    $batch = yellowcube_batch_fetch_inventory_data();
    batch_set($batch);
  }

  if ($form_state['clicked_button']['#name'] == 'send_data') {
    // Create a batch operation.
    $batch = yellowcube_batch_send_data();
    batch_set($batch);
  }

}
