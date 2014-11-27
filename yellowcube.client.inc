<?php
/**
 * @file
 * Glue code for interacting with the yellowcube-php library.
 */

// Load the yellowcube-php via the autoload script provided by composer.
require 'vendor/autoload.php';

use YellowCube\Art\Article;
use YellowCube\Art\ArticleDescription;
use YellowCube\Art\ChangeFlag;
use YellowCube\Art\NetWeight;
use YellowCube\Art\UnitsOfMeasure\AltDenominatorUOM;
use YellowCube\Art\UnitsOfMeasure\AlternateUnitISO;
use YellowCube\Art\UnitsOfMeasure\AltNumeratorUOM;
use YellowCube\Art\UnitsOfMeasure\ISO;
use YellowCube\Art\UnitsOfMeasure\GrossWeight;
use YellowCube\Config;

/**
 * Add a product to YellowCube.
 *
 * @param string $sku
 */
function yellowcube_client_add_product($sku) {
  $product = commerce_product_load_by_sku($sku);

  $client = new YellowCube\Client(Config::testConfig());

  $article = (new Article)
    ->setChangeFlag(ChangeFlag::INSERT)
    ->setPlantID('Y012')
    ->setBaseUOM('PCE')
    ->setNetWeight(new NetWeight(0.042, ISO::KGM))
    ->addUnitOfMeasure(new AlternateUnitISO('PCE'))
    ->addUnitOfMeasure(new AltNumeratorUOM('12'))
    ->addUnitOfMeasure(new AltDenominatorUOM('12'))
    ->addUnitOfMeasure(new GrossWeight(0.5, ISO::KGM))
    ->addArticleDescription(new ArticleDescription('Ich wars nicht.', 'de'))
    ->addArticleDescription(new ArticleDescription('Je te jure! Je n’ai rien fait!', 'fr'))
    ->addArticleDescription(new ArticleDescription('Non sono stato io!', 'it'))
    ->addArticleDescription(new ArticleDescription('It wasn\'t me.', 'en'))
    ->setDepositorNo('0000040750')
    ->setArticleNo($product->sku)
    ->setPeriodExpDateType(1)
    ->setSerialNoFlag(true)
  ;

  $response = $client->insertArticleMasterData($article);
}
