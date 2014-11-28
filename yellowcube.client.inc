<?php
/**
 * @file
 * Glue code for interacting with the yellowcube-php library.
 */

// Load the yellowcube-php via the autoload script provided by composer.
require 'vendor/autoload.php';

use YellowCube\Art\Article;
use YellowCube\Art\ChangeFlag;
use YellowCube\Art\UnitsOfMeasure\ISO;
use YellowCube\WAB\AdditionalService\BasicShippingServices;
use YellowCube\WAB\Order;
use YellowCube\WAB\OrderHeader;
use YellowCube\WAB\Partner;
use YellowCube\WAB\Position;

/**
 * Generate the config object for the YellowCube\Service object.
 *
 * @return \YellowCube\Config
 */
function yellowcube_client_get_config() {
  // Use the testConfig constructor, since we don't want to specify the WSDL manually.
  $config = YellowCube\Config::testConfig();

  return $config;
}

/**
 * Helper function to instantiate the yellowcube-php service object.
 *
 * @return YellowCube\Service
 */
function yellowcube_client_get_service() {
  return new YellowCube\Service(yellowcube_client_get_config());
}

/**
 * Add a product to YellowCube.
 *
 * @param string $sku
 *   SKU of the product to be added.
 */
function yellowcube_client_add_product($sku) {
  $product = commerce_product_load_by_sku($sku);

  $article = new Article;
  $article
    ->setChangeFlag(ChangeFlag::INSERT)
    ->setPlantID(variable_get('yellowcube_plant', ''))
    ->setDepositorNo(variable_get('yellowcube_depositor', ''))
    ->setArticleNo($product->sku)
    ->setBaseUOM(ISO::PCE)
    ->setNetWeight(0.042, ISO::KGM)
    ->setAlternateUnitISO(ISO::PCE)
    ->addArticleDescription($product->title, 'de')
  ;

  $response = yellowcube_client_get_service()->insertArticleMasterData($article);

  assert($response->isSuccess(), 'Inserting article failed.');
}

/**
 * Send order to YellowCube.
 *
 * @param array $ship_addr
 *   Shipping address, in array format available on the customer profile.
 * @param stdClass $order
 *   Drupal Commerce order object.
 *
 * @return bool
 *   TRUE on success, FALSE on failure.
 */
function yellowcube_client_add_order(array $ship_addr, stdClass $order) {
  $partner = new Partner();
  $partner
    ->setPartnerType('WE')
    ->setPartnerNo(variable_get('yellowcube_partner', ''))
    ->setPartnerReference('LiiP AG')
    ->setName1($ship_addr['name_line'])
    ->setName2($ship_addr['premise'])
    ->setStreet($ship_addr['thoroughfare'])
    ->setCountryCode($ship_addr['country'])
    ->setZIPCode($ship_addr['postal_code'])
    ->setCity($ship_addr['locality'])
    ->setEmail($order->mail)
    ->setLanguageCode('en');

  $yc_order = new Order();
  $yc_order
    ->setOrderHeader(new OrderHeader(variable_get('yellowcube_depositor', ''), $order->order_id, date('Ymd', REQUEST_TIME)))
    ->setPartnerAddress($partner)
    ->addValueAddedService(new BasicShippingServices(BasicShippingServices::ECO))
    // XXX: Must be true to work around bug #41: https://trello.com/c/E3hmOIzl
    ->setOrderDocumentsFlag(TRUE);

  foreach ($order->commerce_line_items['und'] as $key => $row) {
    $line_item = commerce_line_item_load($row['line_item_id']);
    $product = commerce_product_load($line_item->commerce_product['und'][0]['product_id']);

    $position = new Position();
    $position
      ->setPosNo($key + 1)
      ->setArticleNo($product->sku)
      ->setPlant(variable_get('yellowcube_plant', ''))
      ->setQuantity($line_item->quantity)
      ->setQuantityISO('PCE')
      ->setShortDescription($product->title_original);

    $yc_order->addOrderPosition($position);
  }

  $response = yellowcube_client_get_service()->createYCCustomerOrder($yc_order);

  $order_link = l(t('view'), 'admin/commerce/orders/' . $order->order_id, array(
    'query' => array('destination' => 'admin/reports/dblog'),
  ));

  if ($response->isSuccess()) {
    watchdog('yellowcube', 'Order #@num was successfully transmitted to YellowCube. Received reference number @ref and status message “@status”.', array(
      '@num' => $order->order_number,
      '@ref' => $response->getReference(),
      '@status' => $response->getStatusText(),
    ), WATCHDOG_NOTICE, $order_link);

    db_query('INSERT INTO {yellowcube_order} (order_id, yc_reference) VALUES (:id, :ref)', array(
      ':id' => $order->order_id,
      ':ref' => $response->getReference(),
    ));

    return TRUE;
  }
  else {
    watchdog('yellowcube', 'Order #@num could not be transmitted to YellowCube: “@status”.', array(
      '@num' => $order->order_number,
      '@status' => $response->getStatusText(),
    ), WATCHDOG_ERROR, $order_link);

    return FALSE;
  }
}
