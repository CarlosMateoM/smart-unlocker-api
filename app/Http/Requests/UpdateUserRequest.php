<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class UpdateUserRequest extends FormRequest
{
    /**
     * Determine if the user is authorized to make this request.
     */
    public function authorize(): bool
    {
        return true;
    }

    /**
     * Get the validation rules that apply to the request.
     *
     * @return array<string, \Illuminate\Contracts\Validation\ValidationRule|array<mixed>|string>
     */
    public function rules(): array
    {
        return [
            'uid'       => 'required|string|unique:users,uid,'. $this->user->id,
            'name'      => 'required|string',
            'email'     => 'required|email|unique:users,email,'. $this->user->id,
            'password'  => 'string|confirmed',
            'role'      => 'required|string|in:admin,user',
        ];
    }
}
