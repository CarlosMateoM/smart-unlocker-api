<?php

use App\Http\Controllers\ActivationRecordController;
use App\Http\Controllers\AuthController;
use App\Http\Controllers\UserController;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;


Route::post('/login', [AuthController::class, 'store']);


Route::middleware('auth:sanctum')->group(function () {
    
    Route::get('/user', function (Request $request) {
        return $request->user();
    });
    
    Route::apiResource('users',                 UserController::class);
    Route::apiResource('activation-records',    ActivationRecordController::class);
    
    Route::post('/logout',                      [AuthController::class, 'destroy']);
});