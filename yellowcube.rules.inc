<?php
/**
 * @file
 * Rules-related code for the YellowCube module.
 */

/**
 * Implements hook_rules_action_info().
 */
function yellowcube_rules_action_info() {
  return array(
    'yellowcube_update_product_action' => array(
      'label' => t('Send product data to YellowCube'),
      'group' => t('YellowCube'),
      'parameter' => array(
        'order' => array(
          'type' => 'commerce_product',
          'label' => t('Product to send'),
        ),
      ),
    ),
    'yellowcube_send_order_action' => array(
      'label' => t('Send order to YellowCube'),
      'group' => t('YellowCube'),
      'parameter' => array(
        'order' => array(
          'type' => 'commerce_order',
          'label' => t('Order to send'),
          // This action modifies the order status, so save it afterwards.
          'save' => TRUE,
        ),
      ),
    ),
  );
}

/**
 * Implementation for yellowcube_update_product_action rules action.
 *
 * @param stdClass $product
 *   Drupal Commerce product object to send to YellowCube.
 */
function yellowcube_update_product_action($product) {
  module_load_include('client.inc', 'yellowcube');

  //Skip hook in batch mode and checkout for performance reasons
  $current_batch = batch_get();
  if (empty($current_batch) && arg(0) !== 'checkout') {
    yellowcube_client_update_product($product->sku);
  }
}

/**
 * Implementation for yellowcube_send_order_action rules action.
 *
 * @param stdClass $order
 *   Drupal Commerce order object to send to YellowCube.
 */
function yellowcube_send_order_action($order) {
  // Only proceed if we have a shipping address.
  if (!empty($order->commerce_customer_shipping['und'][0]['profile_id'])) {
    $shipping_profile = commerce_customer_profile_load($order->commerce_customer_shipping['und'][0]['profile_id']);

    if (!empty($shipping_profile->commerce_customer_address['und'][0])) {
      $ship_addr = $shipping_profile->commerce_customer_address['und'][0];
    }
  }

  // If we couldn't find the required information in the order data, bail out now.
  if (empty($ship_addr)) {
    return;
  }

  module_load_include('client.inc', 'yellowcube');

  $success = yellowcube_client_add_order($ship_addr, $order);

  if ($success) {
    commerce_order_status_update($order, 'yellowcube_received', TRUE);
  }
  else {
    commerce_order_status_update($order, 'yellowcube_send_failed', TRUE);
  }
}
