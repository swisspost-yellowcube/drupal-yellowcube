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
    '#markup' => theme_image(array('path' => drupal_get_path('module', 'yellowcube') . '/img/logo.jpg', 'attributes' => array())),
    '#suffix' => '</div>',
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

  $form['yellowcube_mode'] = array(
    '#type' => 'select',
    '#title' => t('Operating mode'),
    '#options' => array(
      'prod' => t('Production'),
      'int' => t('Integration'),
      'test' => t('Test'),
    ),
    '#default_value' => variable_get('yellowcube_mode', 'test'),
    '#description' => t('If test mode is enabled, orders will get sent to the YellowCube test system. No actual goods will be sent.'),
  );

  return system_settings_form($form);
}

/**
 * YellowCube inventory Management
 */
function yellowcube_inventory_form($form, &$form_state) {

  drupal_set_title(t('YellowCube inventory management'));

  $form['logo'] = array(
    '#prefix' => '<div>',
    '#markup' => theme_image(array('path' => drupal_get_path('module', 'yellowcube') . '/img/logo.jpg', 'attributes' => array())),
    '#suffix' => '</div>',
  );


  $form['send_data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Send product data (ART)'),
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
    '#title' => t('Fetch product data (BAR)'),
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

function yellowcube_inventory_form_submit($form, &$form_state) {
  module_load_include('client.inc', 'yellowcube');

  if($form_state['clicked_button']['#name'] == 'fetch_data') {
    yellowcube_client_get_inventory_data();
  }

  if($form_state['clicked_button']['#name'] == 'send_data') {
    //create a batch operation
    $batch = yellowcube_batch_send_data();
    batch_set($batch);
  }

}