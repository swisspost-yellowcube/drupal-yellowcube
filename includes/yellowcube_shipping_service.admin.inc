<?php

/**
 * @file
 * Contains the administrative page and form callbacks for the Flat Rate module.
 */

function yellowcube_shipping_service_products() {
  $options = array();
  foreach (yellowcube_commerce_products() as $key => $product) {
    $options[$key] = $product['title'];
  }

  return $options;
}

/**
 * Builds the form for adding and editing yellowcube.
 */
function yellowcube_shipping_service_form($form, &$form_state, $shipping_service) {
  $form['#attached']['css'][] = drupal_get_path('module', 'yellowcube') . '/theme/yellowcube.css';

  $add_new_state = array(
    'visible' => array(
      ':input[name="shipping_service[method]"]' => array('value' => 'add_new'),
    ),
  );

  $map_to_existing_state = array(
    'visible' => array(
      ':input[name="shipping_service[method]"]' => array('value' => 'map_to_existing'),
    ),
  );

  // Store the initial shipping service in the form state.
  $form_state['shipping_service'] = $shipping_service;

  $form['shipping_service'] = array(
    '#tree' => TRUE,
  );

  $form['shipping_service']['product'] = array(
    '#type' => 'select',
    '#title' => t('YellowCube Product'),
    '#options' => yellowcube_shipping_service_products(),
    '#description' => t('The YellowCube product you would like to use as shipping service.'),
    '#required' => TRUE,
  );

  $form['shipping_service']['method'] = array(
    '#type' => 'radios',
    '#title' => t('Method'),
    '#options' => array(
      'add_new' => t('Add a new service'),
      'map_to_existing' => t('Map to an existing shipping service'),
    ),
    '#default_value' => 'add_new',
    '#description' => t(''),
    '#required' => TRUE,
  );

  // Add new shipping service

  $form['shipping_service']['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Rate'),
    '#default_value' => '',
//      commerce_currency_amount_to_decimal($shipping_service['base_rate']['amount'], $shipping_service['base_rate']['currency_code']),
//    '#required' => TRUE,
    '#size' => 10,
    '#prefix' => '<div class="yellowcube-shipping-service-rate">',
    '#states' => $add_new_state,
  );

  // Build a currency options list from all enabled currencies.
  $currencies = commerce_currency_get_code(TRUE);
  $form['shipping_service']['currency_code'] = array(
    '#type' => 'select',
    '#title' => t('Currency'),
    '#options' => $currencies,
    '#default_value' => $shipping_service['base_rate']['currency_code'],
    '#suffix' => '</div>',
//    '#required' => TRUE,
    '#states' => $add_new_state,
  );

  $form['shipping_service']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $shipping_service['title'],
    '#description' => t('The administrative title of this shipping service. It is recommended that this title begin with a capital letter and contain only letters, numbers, and spaces.'),
//    '#required' => TRUE,
    '#size' => 32,
    '#maxlength' => 255,
    '#field_suffix' => ' <small id="edit-flat-rate-title-suffix">' . t('Machine name: @name', array('@name' => $shipping_service['name'])) . '</small>',
    '#states' => $add_new_state,
  );

  if (empty($shipping_service['name'])) {
    $form['shipping_service']['name'] = array(
      '#type' => 'machine_name',
      '#title' => t('Machine name'),
      '#default_value' => $shipping_service['name'],
      '#maxlength' => 32,
//      '#required' => TRUE,
      '#machine_name' => array(
        'exists' => 'commerce_shipping_service_load',
        'source' => array('shipping_service', 'title'),
      ),
      '#description' => t('The machine-name of this shipping service. This name must contain only lowercase letters, numbers, and underscores. It must be unique.'),
    );
  }
  else {
    $form['shipping_service']['name'] = array(
      '#type' => 'value',
      '#value' => $shipping_service['name'],
      '#states' => $add_new_state,
    );
  }

  $form['shipping_service']['display_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Display title'),
    '#default_value' => $shipping_service['display_title'],
    '#description' => t('The front end display title of this shipping service shown to customers. Leave blank to default to the <em>Title</em> from above.'),
    '#size' => 32,
    '#states' => $add_new_state,
  );

  // Map to an existing shipping service

  $form['shipping_service']['existing_machine_name'] = array(
    '#type' => 'select',
    '#title' => t('Existing shipping service'),
    '#options' => commerce_shipping_service_get_title(),
    '#description' => t('The shipping service the YellowCube product should be mapped to.'),
//    '#required' => TRUE,
    '#states' => $map_to_existing_state,
  );


  $form['actions'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('form-actions')),
    '#weight' => 40,
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if (!empty($form_state['shipping_service']['name'])) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete shipping service'),
      '#suffix' => l(t('Cancel'), 'admin/commerce/config/shipping/services/yellowcube'),
      '#submit' => array('yellowcube_shipping_service_form_delete_submit'),
      '#weight' => 45,
    );
  }
  else {
    $form['actions']['submit']['#suffix'] = l(t('Cancel'), 'admin/commerce/config/shipping/services/flat-rate');
  }

  return $form;
}

/**
 * Validate handler: ensures a valid base rate was entered for the yellowcube.
 */
function yellowcube_shipping_service_form_validate($form, &$form_state) {
  $required_fields = array(
    'add_new' => array(),
    'map_to_existing' => array(),
  );

  // Ensure the rate amount is numeric.
  if (!is_numeric($form_state['values']['shipping_service']['amount'])) {
    form_set_error('shipping_service][amount', t('You must enter a numeric value for the rate amount.'));
  }
  else {
    // Convert the decimal amount value entered to an integer based amount value.
    $amount = commerce_currency_decimal_to_amount($form_state['values']['shipping_service']['amount'], $form_state['values']['shipping_service']['currency_code']);
    form_set_value( $form['shipping_service']['amount'], $amount, $form_state);
  }
}

/**
 * Submit handler: saves the new or updated shipping service.
 */
function yellowcube_shipping_service_form_submit($form, &$form_state) {
  $shipping_service = $form_state['shipping_service'];

  // Update the shipping service array with values from the form.
  foreach (array('name', 'title', 'display_title', 'amount', 'currency_code', 'method', 'existing_machine_name') as $key) {
    $shipping_service[$key] = $form_state['values']['shipping_service'][$key];
  }

  // If a tax was specified for inclusion, add it to the data array.
  if (!empty($form_state['values']['flat_rate']['include_tax'])) {
    $shipping_service['data']['include_tax'] = $form_state['values']['flat_rate']['include_tax'];
  }
  elseif (!empty($shipping_service['data']['include_tax'])) {
    unset($shipping_service['data']['include_tax']);
  }

  // Save the shipping service.
  unset($shipping_service['base_rate']);
  $op = yellowcube_shipping_service_save($shipping_service);

  if (!$op) {
    drupal_set_message(t('The yellowcube shipping service failed to save properly. Please review the form and try again.'), 'error');
    $form_state['rebuild'] = TRUE;
  }
  else {
    drupal_set_message(t('YellowCube shipping service saved.'));
    $form_state['redirect'] = 'admin/commerce/config/shipping/services/yellowcube';
  }
}

/**
 * Submit handler: redirects to the shipping service delete confirmation form.
 *
 * @see commerce_flat_rate_service_form()
 */
function yellowcube_shipping_service_form_delete_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/commerce/config/shipping/services/yellowcube-' . strtr($form_state['shipping_service']['name'], '_', '-') . '/delete';
}

/**
 * Displays the edit form for an existing shipping service.
 *
 * @param $name
 *   The machine-name of the shipping service to edit.
 */
function commerce_flat_rate_service_edit_page($name) {
  return drupal_get_form('commerce_flat_rate_service_form', commerce_shipping_service_load($name));
}

/**
 * Builds the form for deleting shipping services.
 */
function commerce_flat_rate_service_delete_form($form, &$form_state, $shipping_service) {
  $form_state['shipping_service'] = $shipping_service;

  $form = confirm_form($form,
    t('Are you sure you want to delete the <em>%title</em> service?', array('%title' => $shipping_service['title'])),
    'admin/commerce/config/shipping/services/yellowcube',
    '<p>' . t('This action cannot be undone.') . '</p>',
    t('Delete'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Submit callback for commerce_flat_rate_service_delete_form().
 */
function commerce_flat_rate_service_delete_form_submit($form, &$form_state) {
  $shipping_service = $form_state['shipping_service'];

  commerce_flat_rate_service_delete($shipping_service['name']);

  drupal_set_message(t('The shipping service <em>%title</em> has been deleted.', array('%title' => $shipping_service['title'])));
  watchdog('commerce_flat_rate', 'Deleted shipping service <em>%title</em>.', array('%title' => $shipping_service['title']), WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/commerce/config/shipping/services/flat-rate';
}

/**
 * Displays the delete confirmation form for an existing shipping service.
 *
 * @param $name
 *   The machine-name of the shipping service to delete.
 */
function commerce_flat_rate_service_delete_page($name) {
  return drupal_get_form('commerce_flat_rate_service_delete_form', commerce_shipping_service_load($name));
}
