<?php

namespace App\Http\Controllers;

use App\Http\Requests\StoreUserRequest;
use App\Http\Requests\UpdateUserRequest;
use App\Http\Resources\UserResource;
use App\Models\User;
use Illuminate\Http\Request;
use Spatie\QueryBuilder\QueryBuilder;

class UserController extends Controller
{
    /**
     * Display a listing of the resource.
     */
    public function index()
    {
        $users = QueryBuilder::for(User::class)
            ->allowedFilters(['name', 'email', 'role'])
            ->allowedIncludes('activationRecords')
            ->paginate(15);
        
        return UserResource::collection($users)->resource;
    }

    /**
     * Store a newly created resource in storage.
     */
    public function store(StoreUserRequest $request)
    {
        $user = new User();

        $user->role     = $request->input('role');
        $user->uid      = $request->input('uid');
        $user->name     = $request->input('name');  
        $user->email    = $request->input('email');
        $user->password = bcrypt($request->input('password'));

        $user->save();

        return new UserResource($user);
    }

    /**
     * Display the specified resource.
     */
    public function show(User $user)
    {
        $user->load('activationRecords');

        return new UserResource($user);
    }

    /**
     * Update the specified resource in storage.
     */
    public function update(UpdateUserRequest $request, User $user)
    {
        $user->uid      = $request->input('uid');
        $user->name     = $request->input('name');
        $user->email    = $request->input('email');
        $user->role     = $request->input('role', 'user');
        
        if($request->password) {
            $user->password = bcrypt($request->input('password'));
        }

        $user->save();

        return new UserResource($user);
    }

    /**
     * Remove the specified resource from storage.
     */
    public function destroy(User $user)
    {
        $user->is_enable = !$user->is_enable;   

        $user->save();

        return response()->noContent();
    }
}
