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

  // 'add_new' or 'use_existing'.
  $current_method = !empty($shipping_service['method']) ? $shipping_service['method'] : YELLOWCUBE_SHIPPING_ADD_NEW;

  // Store the initial shipping service in the form state.
  $form_state['shipping_service'] = $shipping_service;

  $form['shipping_service'] = array(
    '#tree' => TRUE,
  );

  $form['shipping_service']['product'] = array(
    '#type' => 'select',
    '#title' => t('YellowCube Product'),
    '#options' => yellowcube_shipping_service_products(),
    '#default_value' => !empty($shipping_service['product']) ? $shipping_service['product'] : null,
    '#description' => t('The YellowCube product you would like to use as shipping service.'),
    '#required' => TRUE,
  );

  $form['shipping_service']['method'] = array(
    '#type' => 'radios',
    '#title' => t('Method'),
    '#options' => array(
      'add_new' => t('Add a new shipping service'),
      'use_existing' => t('Use an existing shipping service'),
    ),
    '#default_value' => $current_method,
    '#description' => t(''),
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => 'yellowcube_shipping_service_method_callback',
      'wrapper' => 'yellowcube-shipping-detail-replace',
      'method' => 'html',
      'effect' => 'slide',
    ),
  );

  $form['detail'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#prefix' => '<div id="yellowcube-shipping-detail-replace">',
    '#suffix' => '</div>',
  );

  $method_form = yellowcube_shipping_service_method_callback($form, $form_state, $current_method);
  $form['detail'] = array_merge($form['detail'], $method_form['detail']);

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
    $form['actions']['submit']['#suffix'] = l(t('Cancel'), 'admin/commerce/config/shipping/services/yellowcube');
  }

  return $form;
}

/**
 * Build the form based on currently selected method.
 *
 * @return array
 * @throws \InvalidArgumentException if no method is given.
 */
function yellowcube_shipping_service_method_callback($form, &$form_state, $default_method = null) {
  $method = !empty($form_state['values']['shipping_service']['method']) ? $form_state['values']['shipping_service']['method'] : $default_method;

  switch($method) {
    case YELLOWCUBE_SHIPPING_ADD_NEW:
      return yellowcube_shipping_service_add_new_form($form, $form_state);
      break;
    case YELLOWCUBE_SHIPPING_USE_EXISTING:
      return yellowcube_shipping_service_use_existing_form($form, $form_state);
    default:
      throw new InvalidArgumentException('Unknown method' . $method);
  }
}

/**
 * Add new form.
 *
 * @return array
 */
function yellowcube_shipping_service_add_new_form($form, &$form_state) {
  $add_new_form = array();
  $shipping_service = $form_state['shipping_service'];

  $add_new_form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Rate'),
    '#default_value' => commerce_currency_amount_to_decimal($shipping_service['base_rate']['amount'], $shipping_service['base_rate']['currency_code']),
    '#required' => TRUE,
    '#size' => 10,
    '#prefix' => '<div class="yellowcube-shipping-service-rate">',
  );

  // Build a currency options list from all enabled currencies.
  $currencies = commerce_currency_get_code(TRUE);
  $add_new_form['currency_code'] = array(
    '#type' => 'select',
    '#title' => t('Currency'),
    '#options' => $currencies,
    '#default_value' => $shipping_service['base_rate']['currency_code'],
    '#suffix' => '</div>',
    '#required' => TRUE,
  );

  $add_new_form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $shipping_service['title'],
    '#description' => t('The administrative title of this shipping service. It is recommended that this title begin with a capital letter and contain only letters, numbers, and spaces.'),
    '#required' => TRUE,
    '#size' => 32,
    '#maxlength' => 255,
    '#field_suffix' => ' <small id="edit-flat-rate-title-suffix">' . t('Machine name: @name', array('@name' => $shipping_service['name'])) . '</small>',
  );

  if (empty($shipping_service['name'])) {
    $add_new_form['name'] = array(
      '#type' => 'machine_name',
      '#title' => t('Machine name'),
      '#default_value' => $shipping_service['name'],
      '#maxlength' => 32,
      '#required' => TRUE,
      '#machine_name' => array(
        'exists' => 'commerce_shipping_service_load',
        'source' => array('detail', 'title'),
      ),
      '#description' => t('The machine-name of this shipping service. This name must contain only lowercase letters, numbers, and underscores. It must be unique.'),
    );
  }
  else {
    $add_new_form['name'] = array(
      '#type' => 'value',
      '#value' => $shipping_service['name'],
    );
  }

  $add_new_form['display_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Display title'),
    '#default_value' => $shipping_service['display_title'],
    '#description' => t('The front end display title of this shipping service shown to customers. Leave blank to default to the <em>Title</em> from above.'),
    '#size' => 32,
  );

  return array('detail' => $add_new_form);
}

/**
 * Use existing form.
 *
 * @return array
 */
function yellowcube_shipping_service_use_existing_form($form, &$form_state) {
  $existing_form = array();
  $shipping_service = $form_state['shipping_service'];

  $existing_form['existing_machine_name'] = array(
    '#type' => 'select',
    '#title' => t('Existing shipping service'),
    '#options' => commerce_shipping_service_get_title(),
    '#default_value' => !empty($shipping_service['existing_machine_name']) ? $shipping_service['existing_machine_name'] : null,
    '#description' => t('The shipping service the YellowCube product should be mapped to.'),
    '#required' => TRUE,
  );

  return array('detail' => $existing_form);
}

/**
 * Validate handler: ensures a valid base rate was entered for the yellowcube.
 */
function yellowcube_shipping_service_form_validate($form, &$form_state) {
  if ($form_state['values']['shipping_service']['method'] === YELLOWCUBE_SHIPPING_USE_EXISTING) {
    return;
  }

  // Ensure the rate amount is numeric.
  if (!empty($form_state['values']['detail']['amount']) && !is_numeric($form_state['values']['detail']['amount'])) {
    form_set_error('detail][amount', t('You must enter a numeric value for the rate amount.'));
  }
  else {
    // Convert the decimal amount value entered to an integer based amount value.
    $amount = commerce_currency_decimal_to_amount($form_state['values']['detail']['amount'], $form_state['values']['detail']['currency_code']);
    form_set_value($form['detail']['amount'], $amount, $form_state);
  }
}

/**
 * Submit handler: saves the new or updated shipping service.
 */
function yellowcube_shipping_service_form_submit($form, &$form_state) {
  $shipping_service = $form_state['shipping_service'];
  $detail = $form_state['values']['detail'];
  $service = $form_state['values']['shipping_service'];

  $shipping_service['product'] = $service['product'];
  $shipping_service['method'] = $service['method'];

  switch ($form_state['values']['shipping_service']['method']) {
    case YELLOWCUBE_SHIPPING_USE_EXISTING:
      $shipping_service['name'] = $service['product'] . '_' . $detail['existing_machine_name'];
      $shipping_service['title'] = $service['product'] . '_' . $detail['existing_machine_name'];
      $shipping_service['existing_machine_name'] = $detail['existing_machine_name'];
      break;
    case YELLOWCUBE_SHIPPING_ADD_NEW:
      foreach (array('name', 'title', 'display_title', 'amount', 'currency_code') as $key) {
        $shipping_service[$key] = $form_state['values']['detail'][$key];
      }
      break;
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
function yellowcube_shipping_service_edit_page($name) {
  return drupal_get_form('yellowcube_shipping_service_form', commerce_shipping_service_load($name));
}

/**
 * Builds the form for deleting shipping services.
 */
function yellowcube_shipping_service_delete_form($form, &$form_state, $shipping_service) {
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
 * Submit callback for yellowcube_shipping_service_delete_form().
 */
function yellowcube_shipping_service_delete_form_submit($form, &$form_state) {
  $shipping_service = $form_state['shipping_service'];

  yellowcube_shipping_service_delete($shipping_service['name']);

  drupal_set_message(t('The shipping service <em>%title</em> has been deleted.', array('%title' => $shipping_service['title'])));
  watchdog('commerce_flat_rate', 'Deleted shipping service <em>%title</em>.', array('%title' => $shipping_service['title']), WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/commerce/config/shipping/services/yellowcube';
}

/**
 * Displays the delete confirmation form for an existing shipping service.
 *
 * @param $name
 *   The machine-name of the shipping service to delete.
 */
function yellowcube_shipping_service_delete_page($name) {
  return drupal_get_form('yellowcube_shipping_service_delete_form', commerce_shipping_service_load($name));
}
