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
use YellowCube\Config;

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
    ->setPlantID('Y012')
    ->setDepositorNo('0000040750')
    ->setArticleNo($product->sku)
    ->setBaseUOM(ISO::PCE)
    ->setNetWeight(0.042, ISO::KGM)
    ->setAlternateUnitISO(ISO::PCE)
    ->addArticleDescription($product->title, 'de')
  ;

  $service = new YellowCube\Service(Config::testConfig());
  $response = $service->insertArticleMasterData($article);

  assert($response->isSuccess(), 'Inserting article failed.');
}
