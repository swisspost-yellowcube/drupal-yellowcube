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
    'yellowcube_send_order_action' => array(
      'label' => t('Send order to YellowCube'),
      'group' => t('YellowCube'),
      'parameter' => array(
        'order' => array(
          'type' => 'commerce_order',
          'label' => t('Order to send'),
        ),
      ),
    ),
  );
}

/**
 * Implementation for yellowcube_send_order_action rules action.
 *
 * @param $order
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

  // TODO: Move this to client.inc.
  return;
  $client = new YellowCube\Service(Config::testConfig());
  $partner = new Partner();
  $partner
    ->setPartnerType('WE')
    ->setPartnerNo('0000300020')
    ->setPartnerReference('LiiP AG')
    ->setTitle('')
    ->setName1($ship_addr['name_line'])
    ->setName2($ship_addr['premise'])
    ->setStreet($ship_addr['thoroughfare'])
    ->setCountryCode($ship_addr['country'])
    ->setZIPCode($ship_addr['postal_code'])
    ->setCity($ship_addr['locality'])
    ->setPOBox('po box')
    ->setPhoneNo('0041793020467')
    ->setSMSAvisMobNo('0041793020467')
    ->setFaxNo('0041793020467')
    ->setEmail($order['mail'])
    ->setLanguageCode('en');

  $order = new Order();
  $order
    ->setOrderHeader(new OrderHeader('0000040730', \uniqid('yc'), date('Ymd')))
    ->setPartnerAddress($partner)
    ->addValueAddedService(new AdditionalShippingServices())
    ->addValueAddedService(new BasicShippingServices(BasicShippingServices::PRI))
    ->addValueAddedService(new DeliveryInstructions(''))
    ->setOrderDocumentsFlag(true);

  foreach ($order->commerce_line_items['und'] as $row) {
    $line_item = commerce_line_item_load($row['line_item_id']);
    $product = commerce_product_load($line_item->commerce_product['und'][0]['product_id']);

    $position = new Position();
    $position
      ->setPosNo(10)
      ->setArticleNo($product->sku)
      ->setPlant('Y010')
      ->setQuantity($line_item->quantity)
      ->setQuantityISO('PCE')
      ->setShortDescription($product->title_original);

    $order->addOrderPosition($position);
  }

  $response = $client->createYCCustomerOrder($order);
  assert($response->isSuccess(), 'Getting status for order failed.');
}
