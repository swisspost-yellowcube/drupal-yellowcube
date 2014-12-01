<?php
/**
 * @file
 * Cron tasks for the YellowCube module.
 */

function yellowcube_jobs_fetch_order_updates() {
  module_load_include('client.inc', 'yellowcube');

  $service = yellowcube_client_get_service();

  $issue_list = $service->getYCCustomerOrderStatus('');

  foreach ($issue_list as $issue) {
    $header = $issue->getCustomerOrderHeader();
    echo "Order number: " . $header->getCustomerOrderNo() . PHP_EOL;
    echo "PostalShipmentNo: " . $header->getPostalShipmentNo() . PHP_EOL;

    foreach ($issue->getCustomerOrderList() as $item) {
      echo "\tPosition: " . $item->getCustomerOrderPosNo();
      echo ", ArticleNo: " . $item->getArticleNo();
      echo PHP_EOL;
    }
  }

  echo PHP_EOL . PHP_EOL;

  print_r($issue_list);
}

/**
 * Cron callback for updating product status.
 */
function yellowcube_jobs_update_products() {
  $queue = DrupalQueue::get('yellowcube_product_status');

  // Select products that either have never been sent to YellowCube or have been
  // changed since they were. Also include any products that were queued more
  // than 6 hours ago, since their processing is then assumed to have failed.
  $update_candidate = db_query("
    SELECT cp.product_id, sku, cp.title, cp.type, cp.created, cp.changed,
           yps.updated AS status_updated, yps.queued
    FROM {commerce_product} AS cp
    LEFT JOIN {yellowcube_product_status} AS yps USING (sku)
    WHERE (yps.article_status = 'enqueued' AND yps.queued < :previous)
          OR yps.article_status IS NULL OR yps.updated < cp.changed
  ", array(':previous' => REQUEST_TIME - 6 * 3600));

  foreach ($update_candidate as $up_cand) {
    $queue->createItem($up_cand->sku);

    db_merge('yellowcube_product_status')
      ->key(array('sku' => $up_cand->sku))
      ->fields(array(
        'article_status' => 'enqueued',
        'queued' => REQUEST_TIME,
      ))
      ->execute();
  }
}
