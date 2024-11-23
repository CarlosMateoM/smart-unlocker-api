<?php

namespace App\Http\Controllers;

use App\Http\Requests\LoginRequest;
use App\Models\User;
use Illuminate\Database\Eloquent\ModelNotFoundException;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Auth;

class AuthController extends Controller
{
    /**
     * Store a newly created resource in storage.
     */
    public function store(LoginRequest $request)
    {
        $user = User::where('email', $request->email)->first();

        if(!$user) {
            throw new ModelNotFoundException('Usuario no encontrado');
        }

        if(!$user->is_enable) {
            return response()->json([
                'message' => 'Usuario deshabilitado'
            ], 401);
        }
        
        $credentials = $request->only('email', 'password');

        if(!Auth::attempt($credentials)) {
            return response()->json([
                'message' => 'email o contraseña incorrectos'
            ], 401);
        }
        
        $token = $user->createToken('auth_token')->plainTextToken;

        return response()->json([
            'access_token' => $token,
        ]);
    }

    /**
     * Remove the specified resource from storage.
     */
    public function destroy(Request $request)
    {
        $request->user()->currentAccessToken()->delete();

        return response()->json([
            'message' => 'Tokens Revocados'
        ]);
    }
}
