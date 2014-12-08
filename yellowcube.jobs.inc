<?php
/**
 * @file
 * Cron tasks for the YellowCube module.
 */

/**
 * Cron job for creating order updates.
 */
function yellowcube_jobs_fetch_order_updates() {
  module_load_include('client.inc', 'yellowcube');

  $service = yellowcube_client_get_service();

  //Find all pending orders
  $pending_orders = yellowcube_jobs_get_pending_orders();

  foreach($pending_orders as $order_ref) {

    //Load the full order object
    $order = commerce_order_load($order_ref->order_id);

    //Check order status
    $response = $service->getYCCustomerOrderStatus($order_ref->yc_reference);

    //Skip problematic orders
    if($response->isError()) {

      $order_link = l(t('view'), 'admin/commerce/orders/' . $order->order_id . '/edit', array(
          'query' => array('destination' => 'admin/reports/dblog'),
        ));

      if ($response->isError()) {
        //log failed orders
        watchdog(
          'yellowcube',
          'Order @order_id could not be transferred to YellowCube. Received reference number @ref and status message “@status”.',
          array(
            '@order_id' => $order->order_number,
            '@ref' => $response->getReference(),
            '@status' => $response->getStatusText(),
          ),
          WATCHDOG_NOTICE,
          $order_link
        );

        commerce_order_status_update($order, 'yellowcube_send_failed', TRUE);

        continue;
      }

      //find orders that have been processed
      if ($response->isSuccess() && !$response->isPending() && !$response->isError()) {

        $goodsIssueList = $service->getYCCustomerOrderReply($order->order_id);

        foreach ($goodsIssueList as $goodsIssue) {
          $header = $goodsIssue->getCustomerOrderHeader();
          $shipment_no = $header->getPostalShipmentNo();
          $shipping_url = 'http://www.post.ch/swisspost-tracking?formattedParcelCodes=' . $shipment_no;

          //Add a message to the order history incl. link to shipping infos
          $message = t('Your order has been shipped. You can use the following url for shipping tracking: !url', array('!url' => $shipping_url));
          yellowcube_jobs_add_custom_message($order, 'completed', $message);
        }

        //Update the order status to complete
        commerce_order_status_update($order, 'complete', TRUE);

      }
    }
  }
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

/**
 * Returns all pending orders on yellow cube
 */
function yellowcube_jobs_get_pending_orders() {

  $pending_orders = db_query("
    SELECT co.order_id, co.order_number, co.status, co.type, co.created, co.changed,
           yo.yc_reference
    FROM {commerce_order} AS co
    LEFT JOIN {yellowcube_order} AS yo USING (order_id)
    WHERE (co.status = 'yellowcube_received');");

  return $pending_orders;
}

/**
 * Add a message to the order
 */

function yellowcube_jobs_add_custom_message($order, $status, $message_body) {
  $statuses = commerce_order_statuses();
  $arguments = array(
    '@order_status_previous' => $statuses[$order->status]['title'],
    '@order_status' => $statuses[$status]['title'],
  );
  $order = commerce_order_status_update($order, $status);

  $message = message_create('commerce_order_state', array('uid' => $order->uid, 'arguments' => $arguments));
  $wrapper = entity_metadata_wrapper('message', $message);
  $wrapper->message_commerce_order->set($order);
  $wrapper->message_commerce_body->set($message_body);
  $wrapper->save();

}