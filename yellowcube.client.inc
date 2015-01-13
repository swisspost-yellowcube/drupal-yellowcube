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

  $operation_mode = variable_get('yellowcube_mode', 'test');

  $cert_path = variable_get('yellowcube_certificate', '');

  $config = new YellowCube\Config(
    variable_get('yellowcube_sender', ''),
    variable_get('yellowcube_endpoint', ''),
    null,
    variable_get('yellowcube_mode', 'T')
  );

  //Certificate handling
  if(in_array($operation_mode, array('P', 'D'))) {
    if (!empty($cert_path) && file_exists($cert_path)) {
      $config->setCertificateFilePath($cert_path);
      //Todo: handle password
    }
  }

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
 * Send product data to YellowCube.
 *
 * @param string $sku
 *   SKU of the product to be added.
 *
 * @return bool
 *   TRUE on success, FALSE on failure.
 */
function yellowcube_client_update_product($sku) {
  $product = commerce_product_load_by_sku($sku);

  $article = new Article;
  $article
    ->setChangeFlag(ChangeFlag::INSERT)
    ->setPlantID(variable_get('yellowcube_plant', ''))
    ->setDepositorNo(variable_get('yellowcube_depositor', ''))
    ->setArticleNo($product->sku)
    ->setBaseUOM(ISO::PCE)
    ->setAlternateUnitISO(ISO::PCE)
    ->addArticleDescription($product->title, 'de');

  //Enable / Disable product based on the status
  switch ($product->status) {
    case FALSE:
      $article->setChangeFlag(ChangeFlag::DEACTIVATE);
      break;

    default:
      $article->setChangeFlag(ChangeFlag::INSERT);
  }

  //Setting the weight
  if (!empty($product->field_weight['und'][0]['weight'])) {
    $weight_unit = $product->field_weight['und'][0]['unit'];

    switch ($weight_unit) {
      case 'kg':
        $weight_unit_iso = ISO::KGM;
        break;
      case 'g':
        $weight_unit_iso = ISO::GRM;
        break;
      default:
        $weight_unit_iso = ISO::KGM;
    }

    $article->setNetWeight(round($product->field_weight['und'][0]['weight'], 3), $weight_unit_iso);
  }

  //Setting the dimensions
  if (!empty($product->field_dimensions['und'][0]['unit'])) {
    $dimension_unit = $product->field_dimensions['und'][0]['unit'];

    switch ($dimension_unit) {
      case 'm':
        $dimension_unit = ISO::MTR;
        break;
      case 'cm':
        $dimension_unit = ISO::CMT;
        break;
      default:
        $dimension_unit = ISO::MTR;
    }

    $article->setLength(round($product->field_dimensions['und'][0]['length'], 3), $dimension_unit);
    $article->setWidth(round($product->field_dimensions['und'][0]['width'], 3), $dimension_unit);
    $article->setHeight(round($product->field_dimensions['und'][0]['height'], 3), $dimension_unit);

  }


  $response = yellowcube_client_get_service()->insertArticleMasterData($article);

  // If we don't get a value in reference1, it means that the product is not
  // yet registered. Change the flag to INSERT and try again.
  if ($response->isSuccess() && !$response->getReference1()) {
    $article->setChangeFlag(ChangeFlag::INSERT);
    $response = yellowcube_client_get_service()->insertArticleMasterData($article);
  }

  $product_link = l(
    t('view'),
    'admin/commerce/products/' . $product->product_id,
    array(
      'query' => array('destination' => 'admin/reports/dblog'),
    )
  );

  if ($response->isSuccess()) {
    watchdog(
      'yellowcube',
      'Product @sku was successfully transmitted to YellowCube. Received reference number @ref and status message “@status”.',
      array(
        '@sku' => $response->getReference1(),
        '@ref' => $response->getReference(),
        '@status' => $response->getStatusText(),
      ),
      WATCHDOG_NOTICE,
      $product_link
    );

    drupal_set_message(
      t(
        'Product @sku was successfully transmitted to YellowCube. Received reference number @ref and status message “@status”.',
        array(
          '@sku' => $response->getReference1(),
          '@ref' => $response->getReference(),
          '@status' => $response->getStatusText(),
        )
      )
    );

    // TODO: save reference ID to the database.

    return TRUE;
  }
  else {
    watchdog(
      'yellowcube',
      'Product @sku could not be transmitted to YellowCube: “@status”.',
      array(
        '@sku' => $response->getReference1(),
        '@status' => $response->getStatusText(),
      ),
      WATCHDOG_ERROR,
      $product_link
    );

    return FALSE;
  }
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
    ->setPartnerReference('Liip AG')
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
    ->setOrderDocumentsFlag(FALSE);

  foreach ($order->commerce_line_items['und'] as $key => $row) {
    $line_item = commerce_line_item_load($row['line_item_id']);

    //Only submit products and skip shipping cost line item
    if (!empty($line_item->commerce_product['und'][0]['product_id'])){
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
  }

  $response = yellowcube_client_get_service()->createYCCustomerOrder($yc_order);

  $order_link = l(
    t('view'),
    'admin/commerce/orders/' . $order->order_id,
    array(
      'query' => array('destination' => 'admin/reports/dblog'),
    )
  );

  if ($response->isSuccess()) {
    watchdog(
      'yellowcube',
      'Order #@num was successfully transmitted to YellowCube. Received reference number @ref and status message “@status”.',
      array(
        '@num' => $order->order_number,
        '@ref' => $response->getReference(),
        '@status' => $response->getStatusText(),
      ),
      WATCHDOG_NOTICE,
      $order_link
    );

    db_query(
      'INSERT INTO {yellowcube_order} (order_id, yc_reference) VALUES (:id, :ref)',
      array(
        ':id' => $order->order_id,
        ':ref' => $response->getReference(),
      )
    );

    return TRUE;
  }
  else {
    watchdog(
      'yellowcube',
      'Order #@num could not be transmitted to YellowCube: “@status”.',
      array(
        '@num' => $order->order_number,
        '@status' => $response->getStatusText(),
      ),
      WATCHDOG_ERROR,
      $order_link
    );

    return FALSE;
  }
}