//----------------------------------------------------------------//
// Copyright (c) 2010-2011 Zipline Games, Inc.
// All Rights Reserved.
// http://getmoai.com
//----------------------------------------------------------------//

#import <moai-core/host.h>
#import <moai-ios/headers.h>
#import <moai-ios-tenjin/MOAITenjinIOSStoreKitListener.h>
#import <moai-ios-tenjin/MOAITenjinIOS.h>

//================================================================//
// MOAITenjinIOSStoreKitListener
//================================================================//
@implementation MOAITenjinIOSStoreKitListener

//================================================================//
#pragma mark -
#pragma mark Protocol SKPaymentTransactionObserver
//================================================================//

//----------------------------------------------------------------//
-( void ) paymentQueue:( SKPaymentQueue* )queue removedTransactions:( NSArray* )transactions {
	UNUSED ( queue );
	UNUSED ( transactions );
}

//----------------------------------------------------------------//
-( void ) paymentQueue:( SKPaymentQueue* )queue restoreCompletedTransactionsFailedWithError:( NSError* )error {
	UNUSED ( queue );
	UNUSED ( error );
	NSLog ( @"StoreKit Restore Completed Transactions Error: %@", error.localizedDescription );
	//MOAIBillingIOS::Get().DidReceivePaymentQueueError(error, "restoreCompletedTransactions");
}

//----------------------------------------------------------------//
-( void ) paymentQueue:( SKPaymentQueue* )queue updatedTransactions:( NSArray* )transactions {
	UNUSED ( queue );
	for ( SKPaymentTransaction* transaction in transactions ) {
		MOAITenjinIOS::Get ().LogPurchaseEvent(transaction);
	}
}

//----------------------------------------------------------------//
-( void ) paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue* )queue {
	UNUSED ( queue );
	NSLog ( @"StoreKit Restore Completed Transactions Finished" );
	//MOAIBillingIOS::Get().DidReceiveRestoreFinished(queue);
}

//================================================================//
#pragma mark -
#pragma mark Protocol SKProductsDelegate
//================================================================//

//----------------------------------------------------------------//
-( void ) request:( SKRequest* )request didFailWithError:( NSError* )error {
	UNUSED ( request );
	
	NSLog ( @"StoreKit Request Error: %@", error.localizedDescription );
	//MOAIBillingIOS::Get().DidReceivePaymentQueueError(error, "request");
}

//================================================================//
#pragma mark -
#pragma mark Protocol SKProductsRequestDelegate
//================================================================//

//----------------------------------------------------------------//
-( void ) productsRequest:( SKProductsRequest* )request didReceiveResponse:( SKProductsResponse* )response {
	
	//MOAIBillingIOS::Get ().ProductsRequestDidReceiveResponse ( request, response );
}

@end
